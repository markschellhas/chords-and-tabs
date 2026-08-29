#include "api/AgentClient.h"
#include "api/AgentHttpServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace chords
{
namespace
{

std::string homeDir()
{
    if (const char* home = std::getenv("HOME"))
        return home;
    return {};
}

std::string defaultConfigDir()
{
#if defined(__APPLE__)
    const auto home = homeDir();
    if (home.empty())
        return "chords-and-tabs";
    return home + "/Library/Application Support/chords-and-tabs";
#else
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg && xdg[0] != '\0')
        return std::string(xdg) + "/chords-and-tabs";
    const auto home = homeDir();
    if (home.empty())
        return "chords-and-tabs";
    return home + "/.config/chords-and-tabs";
#endif
}

std::vector<std::string> searchDirs()
{
    std::vector<std::string> dirs;
    dirs.push_back(agentHomeDir());

    const auto home = homeDir();
    if (! home.empty())
    {
        const std::string xdg = home + "/.config/chords-and-tabs";
        const std::string mac = home + "/Library/Application Support/chords-and-tabs";
        if (xdg != dirs.front())
            dirs.push_back(xdg);
        if (mac != dirs.front())
            dirs.push_back(mac);
    }
    return dirs;
}

int parsePortJson(const std::string& json)
{
    const auto key = json.find("\"port\"");
    if (key == std::string::npos)
        return -1;
    const auto colon = json.find(':', key);
    if (colon == std::string::npos)
        return -1;
    size_t i = colon + 1;
    while (i < json.size() && std::isspace(static_cast<unsigned char>(json[i])))
        ++i;
    char* end = nullptr;
    const long parsed = std::strtol(json.c_str() + static_cast<std::ptrdiff_t>(i), &end, 10);
    if (end == json.c_str() + static_cast<std::ptrdiff_t>(i))
        return -1;
    if (parsed < 1 || parsed > 65535)
        return -1;
    return static_cast<int>(parsed);
}

} // namespace

std::string agentHomeDir()
{
    if (const char* overrideDir = std::getenv("CHORDS_AGENT_HOME"))
    {
        if (overrideDir[0] != '\0')
            return overrideDir;
    }
    return defaultConfigDir();
}

bool writeAgentSnapshot(const std::string& fileName, const std::string& contents)
{
    std::error_code ec;
    const auto dir = std::filesystem::path(agentHomeDir());
    std::filesystem::create_directories(dir, ec);
    if (ec)
        return false;

    const auto path = dir / fileName;
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (! out)
        return false;
    out << contents;
    return static_cast<bool>(out);
}

std::optional<std::string> readAgentSnapshot(const std::string& fileName)
{
    for (const auto& dir : searchDirs())
    {
        const auto path = std::filesystem::path(dir) / fileName;
        std::ifstream in(path, std::ios::binary);
        if (! in)
            continue;
        std::ostringstream oss;
        oss << in.rdbuf();
        return oss.str();
    }
    return std::nullopt;
}

int discoverAgentPort()
{
    if (const char* env = std::getenv("CHORDS_AGENT_PORT"))
    {
        char* end = nullptr;
        const long parsed = std::strtol(env, &end, 10);
        if (end != env && parsed >= 0 && parsed <= 65535)
            return static_cast<int>(parsed);
    }

    if (const auto meta = readAgentSnapshot("agent-api.json"))
    {
        const int recorded = parsePortJson(*meta);
        if (recorded > 0)
            return recorded;
    }

    return AgentHttpServer::kDefaultPort;
}

HttpResult httpGetLocal(int port, const std::string& path)
{
    HttpResult result;
    if (port <= 0)
        return result;

    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return result;

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        ::close(fd);
        return result;
    }

    const auto req = "GET " + path + " HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
    if (::send(fd, req.data(), req.size(), MSG_NOSIGNAL) < 0)
    {
        ::close(fd);
        return result;
    }

    std::string response;
    char buf[2048];
    for (;;)
    {
        const ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
        if (n <= 0)
            break;
        response.append(buf, static_cast<size_t>(n));
    }
    ::close(fd);

    if (response.rfind("HTTP/", 0) != 0)
        return result;

    char* end = nullptr;
    result.status = static_cast<int>(std::strtol(response.c_str() + 9, &end, 10));
    const auto sep = response.find("\r\n\r\n");
    if (sep != std::string::npos)
        result.body = response.substr(sep + 4);
    return result;
}

std::optional<AgentDocument> readAgentDocument(const std::string& route,
                                               const std::string& snapshotName)
{
    const int port = discoverAgentPort();
    const auto live = httpGetLocal(port, route);
    if (live.status == 200 && ! live.body.empty())
        return AgentDocument { live.body, AgentSource::Live, port };

    if (auto snap = readAgentSnapshot(snapshotName))
        return AgentDocument { std::move(*snap), AgentSource::Snapshot, port };

    return std::nullopt;
}

bool agentIsLive()
{
    const auto live = httpGetLocal(discoverAgentPort(), "/health");
    return live.status == 200 && live.body.find("\"ok\":true") != std::string::npos;
}

} // namespace chords
