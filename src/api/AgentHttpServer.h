#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace chords
{

/**
 * Loopback-only HTTP/1.1 server so a local agent can read the live song.
 * Serves prebuilt JSON snapshots; the audio/UI thread never blocks on I/O.
 *
 *   GET /              catalog
 *   GET /health        liveness
 *   GET /song          full song (empty slots are null)
 *   GET /progressions  chords that have been added
 */
class AgentHttpServer
{
public:
    static constexpr int kDefaultPort = 17891;

    AgentHttpServer();
    ~AgentHttpServer();

    AgentHttpServer(const AgentHttpServer&) = delete;
    AgentHttpServer& operator=(const AgentHttpServer&) = delete;

    /** Default 17891, or $CHORDS_AGENT_PORT when set. Port 0 binds an ephemeral port. */
    static int defaultPort();

    bool start(int port = -1);
    void stop();

    bool running() const { return running_.load(); }
    int port() const { return port_.load(); }

    void setSongJson(std::string json);
    void setProgressionsJson(std::string json);

private:
    void loop();
    void handleClient(int clientFd);
    std::string documentFor(const std::string& path) const;

    std::atomic<bool> running_ { false };
    std::atomic<int> port_ { 0 };
    std::atomic<int> listenFd_ { -1 };
    std::thread thread_;

    mutable std::mutex mutex_;
    std::string songJson_ { "{}" };
    std::string progressionsJson_ { "{}" };
};

} // namespace chords
