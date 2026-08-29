#include "api/AgentHttpServer.h"
#include "api/SongJson.h"
#include "model/Song.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>

namespace
{

std::string httpGet(int port, const std::string& path)
{
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return {};

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        ::close(fd);
        return {};
    }

    const auto req = "GET " + path + " HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
    if (::send(fd, req.data(), req.size(), 0) < 0)
    {
        ::close(fd);
        return {};
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

    const auto sep = response.find("\r\n\r\n");
    if (sep == std::string::npos)
        return {};
    return response.substr(sep + 4);
}

bool contains(const std::string& hay, const char* needle)
{
    return hay.find(needle) != std::string::npos;
}

} // namespace

int main()
{
    int failures = 0;
    auto fail = [&](const char* msg) {
        std::cerr << "FAIL " << msg << "\n";
        ++failures;
    };

    using namespace chords;

    Song song;
    const AgentView view { 0 };
    const auto songJson = songToJson(song, view);
    const auto progJson = progressionsToJson(song, view);

    if (! contains(songJson, "\"major\":\"C\"")) fail("song key C");
    if (! contains(songJson, "\"name\":\"Verse\"")) fail("song verse");
    if (! contains(songJson, "\"name\":\"Chorus\"")) fail("song chorus");
    if (! contains(songJson, "\"name\":\"C\"")) fail("song starter C");
    if (! contains(songJson, "\"numeral\":\"I\"")) fail("song roman I");

    if (! contains(progJson, "\"progression\":\"C | G F C | Dm\""))
        fail("verse progression text");
    if (! contains(progJson, "\"progression\":\"D | G | C | Em\""))
        fail("chorus progression text");
    if (! contains(progJson, "\"name\":\"Dm\"") || ! contains(progJson, "\"numeral\":\"ii\""))
        fail("Dm is ii in C");
    if (! contains(progJson, "\"bar\":1") || ! contains(progJson, "\"slot\":2"))
        fail("split-bar slot index");

    song.setChord(0, 0, 0, std::nullopt);
    const auto afterClear = progressionsToJson(song, view);
    if (contains(afterClear, "\"progression\":\"C |"))
        fail("cleared first chord should be a rest");
    if (! contains(afterClear, "\"progression\":\"- | G F C | Dm\""))
        fail("rest shown as dash");

    song.setSectionName(0, "Verse \"A\"");
    const auto escaped = songToJson(song, view);
    if (! contains(escaped, "\"name\":\"Verse \\\"A\\\"\""))
        fail("json escape quotes in section name");

    AgentHttpServer server;
    if (! server.start(0))
        fail("server start");
    else
    {
        server.setSongJson(songToJson(song, view));
        server.setProgressionsJson(progressionsToJson(song, view));

        const int port = server.port();
        if (port <= 0) fail("ephemeral port");

        const auto health = httpGet(port, "/health");
        if (! contains(health, "\"ok\":true")) fail("health ok");

        const auto catalog = httpGet(port, "/");
        if (! contains(catalog, "/progressions")) fail("catalog lists progressions");

        const auto live = httpGet(port, "/progressions");
        if (! contains(live, "\"progression\":\"- | G F C | Dm\""))
            fail("GET /progressions live snapshot");

        const auto full = httpGet(port, "/song");
        if (! contains(full, "\"name\":\"Chorus\"")) fail("GET /song");

        const auto missing = httpGet(port, "/nope");
        if (! contains(missing, "not found")) fail("404 body");

        server.stop();
        if (server.running()) fail("stopped");
    }

    if (failures == 0)
        std::cout << "AgentApiTest: OK\n";
    return failures == 0 ? 0 : 1;
}
