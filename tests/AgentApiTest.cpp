#include "api/AgentClient.h"
#include "api/AgentHttpServer.h"
#include "api/SongJson.h"
#include "model/Song.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h>

namespace
{

bool contains(const std::string& hay, const char* needle)
{
    return hay.find(needle) != std::string::npos;
}

std::string uniqueHome()
{
    return (std::filesystem::temp_directory_path()
            / ("chords-agent-test-" + std::to_string(::getpid()))).string();
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
    if (! contains(songJson, "\"rowRepeats\":[false]")) fail("song default rowRepeats");
    if (! contains(progJson, "\"rowRepeats\":[false]")) fail("progressions default rowRepeats");

    song.setRowRepeat(0, 0, true);
    const auto repeated = progressionsToJson(song, view);
    if (! contains(repeated, "\"rowRepeats\":[true]")) fail("progressions repeat on");
    song.setRowRepeat(0, 0, false);

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

    const auto home = uniqueHome();
    ::setenv("CHORDS_AGENT_HOME", home.c_str(), 1);
    std::filesystem::remove_all(home);

    AgentHttpServer server;
    if (! server.start(0))
        fail("server start");
    else
    {
        const auto liveProg = progressionsToJson(song, view);
        const auto liveSong = songToJson(song, view);
        server.setSongJson(liveSong);
        server.setProgressionsJson(liveProg);

        const int port = server.port();
        if (port <= 0) fail("ephemeral port");
        ::setenv("CHORDS_AGENT_PORT", std::to_string(port).c_str(), 1);

        writeAgentSnapshot("song.json", liveSong);
        writeAgentSnapshot("progressions.json", liveProg);
        writeAgentSnapshot("agent-api.json",
                           std::string("{\"port\":") + std::to_string(port) + "}");

        const auto health = httpGetLocal(port, "/health");
        if (health.status != 200 || ! contains(health.body, "\"ok\":true"))
            fail("health ok");

        const auto catalog = httpGetLocal(port, "/");
        if (! contains(catalog.body, "/progressions")) fail("catalog lists progressions");

        const auto live = readAgentDocument("/progressions", "progressions.json");
        if (! live || live->source != AgentSource::Live)
            fail("prefers live API");
        if (! live || ! contains(live->body, "\"progression\":\"- | G F C | Dm\""))
            fail("GET /progressions live snapshot");

        const auto full = httpGetLocal(port, "/song");
        if (! contains(full.body, "\"name\":\"Chorus\"")) fail("GET /song");

        const auto missing = httpGetLocal(port, "/nope");
        if (missing.status != 404) fail("404 status");

        if (! agentIsLive()) fail("agentIsLive while serving");

        server.stop();
        if (server.running()) fail("stopped");
        if (agentIsLive()) fail("not live after stop");

        const auto snap = readAgentDocument("/progressions", "progressions.json");
        if (! snap || snap->source != AgentSource::Snapshot)
            fail("falls back to snapshot");
        if (! snap || ! contains(snap->body, "\"name\":\"Dm\""))
            fail("snapshot body");
    }

    std::filesystem::remove_all(home);

    if (failures == 0)
        std::cout << "AgentApiTest: OK\n";
    return failures == 0 ? 0 : 1;
}
