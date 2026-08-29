#pragma once

#include <optional>
#include <string>

namespace chords
{

enum class AgentSource
{
    Live,
    Snapshot
};

struct AgentDocument
{
    std::string body;
    AgentSource source = AgentSource::Snapshot;
    int port = 0;
};

struct HttpResult
{
    int status = 0; // 0 = connection failed
    std::string body;
};

/** Override with $CHORDS_AGENT_HOME; otherwise the platform config dir. */
std::string agentHomeDir();

bool writeAgentSnapshot(const std::string& fileName, const std::string& contents);
std::optional<std::string> readAgentSnapshot(const std::string& fileName);

/** $CHORDS_AGENT_PORT, else port recorded in agent-api.json, else 17891. */
int discoverAgentPort();

HttpResult httpGetLocal(int port, const std::string& path);

/**
 * Live loopback document, then last-written snapshot.
 * `route` is the HTTP path ("/progressions"); `snapshotName` is the file.
 */
std::optional<AgentDocument> readAgentDocument(const std::string& route,
                                               const std::string& snapshotName);

bool agentIsLive();

} // namespace chords
