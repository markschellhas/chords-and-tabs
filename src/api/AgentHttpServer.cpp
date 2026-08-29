#include "api/AgentHttpServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <utility>

namespace chords
{
namespace
{

constexpr int kMaxRequestBytes = 8192;

std::string httpResponse(int status, const char* reason, const std::string& body,
                         const char* contentType)
{
    std::ostringstream os;
    os << "HTTP/1.1 " << status << ' ' << reason << "\r\n"
       << "Content-Type: " << contentType << "\r\n"
       << "Content-Length: " << body.size() << "\r\n"
       << "Connection: close\r\n"
       << "Cache-Control: no-store\r\n"
       << "Access-Control-Allow-Origin: *\r\n"
       << "\r\n"
       << body;
    return os.str();
}

std::string jsonResponse(int status, const char* reason, const std::string& body)
{
    return httpResponse(status, reason, body, "application/json; charset=utf-8");
}

bool writeAll(int fd, const std::string& data)
{
    size_t sent = 0;
    while (sent < data.size())
    {
        const ssize_t n = ::send(fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            return false;
        }
        if (n == 0)
            return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

} // namespace

AgentHttpServer::AgentHttpServer() = default;

AgentHttpServer::~AgentHttpServer()
{
    stop();
}

int AgentHttpServer::defaultPort()
{
    if (const char* env = std::getenv("CHORDS_AGENT_PORT"))
    {
        char* end = nullptr;
        const long parsed = std::strtol(env, &end, 10);
        if (end != env && parsed >= 0 && parsed <= 65535)
            return static_cast<int>(parsed);
    }
    return kDefaultPort;
}

bool AgentHttpServer::start(int port)
{
    stop();

    if (port < 0)
        port = defaultPort();

    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return false;

    const int yes = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        ::close(fd);
        return false;
    }

    if (::listen(fd, 8) != 0)
    {
        ::close(fd);
        return false;
    }

    sockaddr_in bound {};
    socklen_t boundLen = sizeof(bound);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&bound), &boundLen) != 0)
    {
        ::close(fd);
        return false;
    }

    listenFd_.store(fd);
    port_.store(ntohs(bound.sin_port));
    running_.store(true);
    thread_ = std::thread([this] { loop(); });
    return true;
}

void AgentHttpServer::stop()
{
    if (! running_.exchange(false) && listenFd_.load() < 0)
        return;

    const int fd = listenFd_.exchange(-1);
    if (fd >= 0)
        ::close(fd);

    if (thread_.joinable())
        thread_.join();

    port_.store(0);
}

void AgentHttpServer::setSongJson(std::string json)
{
    std::lock_guard<std::mutex> lock(mutex_);
    songJson_ = std::move(json);
}

void AgentHttpServer::setProgressionsJson(std::string json)
{
    std::lock_guard<std::mutex> lock(mutex_);
    progressionsJson_ = std::move(json);
}

void AgentHttpServer::loop()
{
    while (running_.load())
    {
        const int fd = listenFd_.load();
        if (fd < 0)
            break;

        pollfd pfd {};
        pfd.fd = fd;
        pfd.events = POLLIN;
        const int ready = ::poll(&pfd, 1, 200);
        if (ready < 0)
        {
            if (errno == EINTR)
                continue;
            break;
        }
        if (ready == 0)
            continue;
        if ((pfd.revents & POLLIN) == 0)
            break;

        const int client = ::accept(fd, nullptr, nullptr);
        if (client < 0)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            if (! running_.load())
                break;
            continue;
        }
        handleClient(client);
        ::close(client);
    }
}

void AgentHttpServer::handleClient(int clientFd)
{
    std::string request;
    request.reserve(512);
    char buf[1024];

    while (request.size() < static_cast<size_t>(kMaxRequestBytes))
    {
        const ssize_t n = ::recv(clientFd, buf, sizeof(buf), 0);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            return;
        }
        if (n == 0)
            break;
        request.append(buf, static_cast<size_t>(n));
        if (request.find("\r\n\r\n") != std::string::npos)
            break;
    }

    const auto lineEnd = request.find("\r\n");
    if (lineEnd == std::string::npos)
    {
        writeAll(clientFd, jsonResponse(400, "Bad Request", "{\"error\":\"bad request\"}"));
        return;
    }

    std::istringstream line(request.substr(0, lineEnd));
    std::string method, target, version;
    line >> method >> target >> version;

    const auto qpos = target.find('?');
    const std::string path = qpos == std::string::npos ? target : target.substr(0, qpos);

    if (method != "GET" && method != "HEAD")
    {
        writeAll(clientFd, jsonResponse(405, "Method Not Allowed",
                                        "{\"error\":\"only GET is supported\"}"));
        return;
    }

    const std::string body = documentFor(path);
    if (body.empty())
    {
        writeAll(clientFd, jsonResponse(404, "Not Found", "{\"error\":\"not found\"}"));
        return;
    }

    auto response = jsonResponse(200, "OK", body);
    if (method == "HEAD")
    {
        const auto sep = response.find("\r\n\r\n");
        if (sep != std::string::npos)
            response.resize(sep + 4);
    }
    writeAll(clientFd, response);
}

std::string AgentHttpServer::documentFor(const std::string& path) const
{
    if (path == "/" || path == "/index.json")
    {
        std::ostringstream os;
        os << "{\"app\":\"chords-and-tabs\","
           << "\"port\":" << port_.load() << ","
           << "\"endpoints\":{"
           << "\"/health\":\"liveness\","
           << "\"/song\":\"full song including empty slots\","
           << "\"/progressions\":\"chords that have been added, by section\""
           << "}}";
        return os.str();
    }
    if (path == "/health")
    {
        std::ostringstream os;
        os << "{\"ok\":true,\"app\":\"chords-and-tabs\",\"port\":" << port_.load() << "}";
        return os.str();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (path == "/song")
        return songJson_;
    if (path == "/progressions")
        return progressionsJson_;
    return {};
}

} // namespace chords
