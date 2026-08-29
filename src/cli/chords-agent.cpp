#include "api/AgentClient.h"

#include <cstring>
#include <iostream>
#include <string>

namespace
{

void usage(std::ostream& out)
{
    out << "chords-agent — read chord progressions from a running chords-and-tabs song\n"
        << "\n"
        << "Usage:\n"
        << "  chords-agent progressions [--live]\n"
        << "  chords-agent song         [--live]\n"
        << "  chords-agent health\n"
        << "\n"
        << "progressions  Chords that have been placed, grouped by section.\n"
        << "song          Full song document; empty slots are null.\n"
        << "health        Whether the app is serving live state (exit 0 / 2).\n"
        << "\n"
        << "By default, progressions/song use the live loopback API, then the\n"
        << "last snapshot under $CHORDS_AGENT_HOME (or ~/.config/chords-and-tabs).\n"
        << "--live fails if the app is not running.\n"
        << "\n"
        << "Port: $CHORDS_AGENT_PORT, else agent-api.json, else 17891.\n";
}

int printDocument(const char* route, const char* snapshot, bool liveOnly)
{
    if (liveOnly)
    {
        const int port = chords::discoverAgentPort();
        const auto live = chords::httpGetLocal(port, route);
        if (live.status != 200 || live.body.empty())
        {
            std::cerr << "chords-agent: app is not reachable on 127.0.0.1:" << port << "\n";
            return 2;
        }
        std::cout << live.body;
        if (live.body.empty() || live.body.back() != '\n')
            std::cout << '\n';
        return 0;
    }

    const auto doc = chords::readAgentDocument(route, snapshot);
    if (! doc)
    {
        std::cerr << "chords-agent: no live API and no snapshot for " << snapshot << "\n";
        return 2;
    }
    std::cout << doc->body;
    if (doc->body.empty() || doc->body.back() != '\n')
        std::cout << '\n';
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        usage(std::cerr);
        return 1;
    }

    bool liveOnly = false;
    const char* command = nullptr;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0)
        {
            usage(std::cout);
            return 0;
        }
        if (std::strcmp(argv[i], "--live") == 0)
        {
            liveOnly = true;
            continue;
        }
        if (argv[i][0] == '-')
        {
            std::cerr << "chords-agent: unknown option " << argv[i] << "\n";
            return 1;
        }
        if (command != nullptr)
        {
            usage(std::cerr);
            return 1;
        }
        command = argv[i];
    }

    if (command == nullptr)
    {
        usage(std::cerr);
        return 1;
    }

    const std::string cmd = command;
    if (cmd == "progressions")
        return printDocument("/progressions", "progressions.json", liveOnly);
    if (cmd == "song")
        return printDocument("/song", "song.json", liveOnly);
    if (cmd == "health")
    {
        if (chords::agentIsLive())
        {
            const int port = chords::discoverAgentPort();
            std::cout << "{\"ok\":true,\"port\":" << port << "}\n";
            return 0;
        }
        std::cout << "{\"ok\":false}\n";
        return 2;
    }

    std::cerr << "chords-agent: unknown command " << cmd << "\n";
    usage(std::cerr);
    return 1;
}
