#include "realms.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace glitchcraft {
namespace {

#ifdef _WIN32
using Socket = SOCKET;
constexpr Socket kInvalidSocket = INVALID_SOCKET;
int socketError() { return WSAGetLastError(); }
void closeSocket(Socket socket) { closesocket(socket); }
bool setNonblocking(Socket socket)
{
    u_long enabled = 1;
    return ioctlsocket(socket, FIONBIO, &enabled) == 0;
}
#else
using Socket = int;
constexpr Socket kInvalidSocket = -1;
int socketError() { return errno; }
void closeSocket(Socket socket) { close(socket); }
bool setNonblocking(Socket socket)
{
    const int flags = fcntl(socket, F_GETFL, 0);
    return flags >= 0 && fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
}
#endif

constexpr std::size_t kMaxLineLength = 512;
constexpr std::size_t kMaxQueuedBytes = 64U * 1024U;
constexpr int kMaxClients = 8;

bool validPosition(float x, float y, float z)
{
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z) &&
           std::abs(x) < 1000000.0F && std::abs(y) < 1000000.0F &&
           std::abs(z) < 1000000.0F;
}

bool waitReadable(Socket socket, int timeoutMilliseconds)
{
    fd_set readable;
    FD_ZERO(&readable);
    FD_SET(socket, &readable);
    timeval timeout{};
    timeout.tv_sec = timeoutMilliseconds / 1000;
    timeout.tv_usec = (timeoutMilliseconds % 1000) * 1000;
#ifdef _WIN32
    return select(0, &readable, nullptr, nullptr, &timeout) > 0;
#else
    return select(socket + 1, &readable, nullptr, nullptr, &timeout) > 0;
#endif
}

bool readHandshakeLine(Socket socket, std::string &line)
{
    line.clear();
    while (line.size() < kMaxLineLength) {
        if (!waitReadable(socket, 5000)) {
            return false;
        }
        char byte = 0;
#ifdef _WIN32
        const int received = recv(socket, &byte, 1, 0);
#else
        const auto received = recv(socket, &byte, 1, 0);
#endif
        if (received <= 0) {
            return false;
        }
        if (byte == '\n') {
            return true;
        }
        if (byte != '\r') {
            line.push_back(byte);
        }
    }
    return false;
}

void appendLine(std::string &queue, const std::string &line)
{
    if (line.size() + 1U > kMaxQueuedBytes - queue.size()) {
        queue.clear();
        return;
    }
    queue.append(line);
    queue.push_back('\n');
}

std::string positionMessage(int id, const Game &game)
{
    std::ostringstream message;
    message.precision(6);
    message << "P " << id << ' ' << game.playerX() << ' ' << game.playerY()
            << ' ' << game.playerZ() << ' ' << game.yaw() << ' '
            << game.pitch();
    return message.str();
}

}

struct LanSession::Impl {
    struct Peer {
        Socket socket = kInvalidSocket;
        int id = 0;
        bool handshaken = false;
        std::string input;
        std::string output;
    };

    Socket listener = kInvalidSocket;
    Socket connection = kInvalidSocket;
    std::vector<Peer> peers;
    std::uint32_t seed = 0;
    std::uint16_t hostPort = kDefaultPort;
    int localId = 1;
    int nextId = 2;
    bool host = false;
    bool failed = false;
    std::string lastError;
    std::string clientInput;
    std::string clientOutput;
    std::chrono::steady_clock::time_point lastPosition{};

    ~Impl()
    {
        if (listener != kInvalidSocket) {
            closeSocket(listener);
        }
        if (connection != kInvalidSocket) {
            closeSocket(connection);
        }
        for (const Peer &peer : peers) {
            if (peer.socket != kInvalidSocket) {
                closeSocket(peer.socket);
            }
        }
#ifdef _WIN32
        WSACleanup();
#endif
    }

    void fail(const std::string &message)
    {
        failed = true;
        lastError = message;
    }

    void queueClient(const std::string &line)
    {
        if (clientOutput.size() + line.size() + 1U > kMaxQueuedBytes) {
            fail("LAN send queue exceeded its safety limit.");
            return;
        }
        appendLine(clientOutput, line);
    }

    void queuePeer(Peer &peer, const std::string &line)
    {
        if (peer.output.size() + line.size() + 1U > kMaxQueuedBytes) {
            peer.output.clear();
            peer.output = "ERROR send queue limit exceeded\n";
            return;
        }
        appendLine(peer.output, line);
    }

    void broadcast(const std::string &line, int excludedId = 0)
    {
        for (Peer &peer : peers) {
            if (peer.id != excludedId) {
                queuePeer(peer, line);
            }
        }
    }

    void acceptClients(Game &game)
    {
        for (;;) {
            sockaddr_storage address{};
#ifdef _WIN32
            int addressSize = sizeof(address);
#else
            socklen_t addressSize = sizeof(address);
#endif
            const Socket client = accept(
                listener, reinterpret_cast<sockaddr *>(&address), &addressSize);
            if (client == kInvalidSocket) {
                break;
            }
            if (peers.size() >= static_cast<std::size_t>(kMaxClients) ||
                !setNonblocking(client)) {
                closeSocket(client);
                continue;
            }
            Peer peer;
            peer.socket = client;
            peer.id = nextId++;
            peer.output = "WELCOME 1 " + std::to_string(seed) + " " +
                          std::to_string(peer.id) + "\n";
            for (const auto &entry : game.blockEdits()) {
                std::ostringstream message;
                message << "B " << entry.first.x << ' ' << entry.first.y
                        << ' ' << entry.first.z << ' '
                        << static_cast<int>(entry.second);
                appendLine(peer.output, message.str());
            }
            for (const RemotePlayer &remote : game.remotePlayers()) {
                std::ostringstream message;
                message.precision(6);
                message << "P " << remote.id << ' ' << remote.x << ' '
                        << remote.y << ' ' << remote.z << ' ' << remote.yaw
                        << ' ' << remote.pitch;
                appendLine(peer.output, message.str());
            }
            appendLine(peer.output, positionMessage(localId, game));
            peers.push_back(std::move(peer));
        }
    }

    bool applyMessage(const std::string &line, Game &game, int senderId)
    {
        std::istringstream input(line);
        char type = '\0';
        input >> type;
        if (type == 'B') {
            int x = 0;
            int y = 0;
            int z = 0;
            int block = 0;
            std::string extra;
            if (!(input >> x >> y >> z >> block) || (input >> extra) ||
                y < -64 || y > 255 ||
                block < GC_BLOCK_AIR || block >= GC_BLOCK_COUNT ||
                !game.applyBlockEdit(x, y, z, static_cast<GcBlock>(block))) {
                return false;
            }
            std::ostringstream canonical;
            canonical << "B " << x << ' ' << y << ' ' << z << ' ' << block;
            if (host) {
                broadcast(canonical.str(), senderId);
            }
            return true;
        }
        if (type == 'P') {
            int id = 0;
            float x = 0.0F;
            float y = 0.0F;
            float z = 0.0F;
            float yaw = 0.0F;
            float pitch = 0.0F;
            std::string extra;
            if (!(input >> id >> x >> y >> z >> yaw >> pitch) ||
                (input >> extra) || (!host && id <= 0) ||
                !validPosition(x, y, z) || !std::isfinite(yaw) ||
                !std::isfinite(pitch)) {
                return false;
            }
            if (host) {
                id = senderId;
            }
            game.setRemotePlayer(id, x, y, z, yaw, pitch);
            if (host) {
                std::ostringstream canonical;
                canonical.precision(6);
                canonical << "P " << id << ' ' << x << ' ' << y << ' ' << z
                          << ' ' << yaw << ' ' << pitch;
                broadcast(canonical.str(), senderId);
            }
            return true;
        }
        if (!host && type == 'D') {
            int id = 0;
            std::string extra;
            if (!(input >> id) || (input >> extra) || id <= 1) {
                return false;
            }
            game.removeRemotePlayer(id);
            return true;
        }
        return false;
    }

    void readPeerMessages(Game &game, Peer &peer)
    {
        char buffer[1024];
        for (;;) {
#ifdef _WIN32
            const int received = recv(peer.socket, buffer,
                                      static_cast<int>(sizeof(buffer)), 0);
#else
            const auto received = recv(peer.socket, buffer, sizeof(buffer), 0);
#endif
            if (received == 0) {
                peer.socket = kInvalidSocket;
                game.removeRemotePlayer(peer.id);
                return;
            }
            if (received < 0) {
#ifdef _WIN32
                if (socketError() != WSAEWOULDBLOCK) {
#else
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
#endif
                    peer.socket = kInvalidSocket;
                    game.removeRemotePlayer(peer.id);
                }
                return;
            }
            peer.input.append(buffer, static_cast<std::size_t>(received));
            if (peer.input.size() > kMaxLineLength * 16U) {
                peer.socket = kInvalidSocket;
                game.removeRemotePlayer(peer.id);
                return;
            }
            std::size_t newline = peer.input.find('\n');
            while (newline != std::string::npos) {
                std::string line = peer.input.substr(0, newline);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                if (!line.empty()) {
                    if (!peer.handshaken) {
                        if (line != "HELLO 1") {
                            peer.socket = kInvalidSocket;
                            return;
                        }
                        peer.handshaken = true;
                    } else if (!applyMessage(line, game, peer.id)) {
                        peer.socket = kInvalidSocket;
                        game.removeRemotePlayer(peer.id);
                        return;
                    }
                }
                peer.input.erase(0, newline + 1U);
                newline = peer.input.find('\n');
            }
        }
    }

    void flush(Socket socket, std::string &queue)
    {
        if (queue.empty()) {
            return;
        }
#ifdef _WIN32
        const int sent = send(socket, queue.data(),
                              static_cast<int>(queue.size()), 0);
        if (sent > 0) {
            queue.erase(0, static_cast<std::size_t>(sent));
        } else if (sent < 0 && socketError() != WSAEWOULDBLOCK) {
            fail("LAN connection was lost while sending.");
        }
#else
        const auto sent = send(socket, queue.data(), queue.size(), MSG_NOSIGNAL);
        if (sent > 0) {
            queue.erase(0, static_cast<std::size_t>(sent));
        } else if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK &&
                   errno != EINTR) {
            fail("LAN connection was lost while sending.");
        }
#endif
    }

    void readClientMessages(Game &game)
    {
        char buffer[1024];
        for (;;) {
#ifdef _WIN32
            const int received = recv(connection, buffer,
                                      static_cast<int>(sizeof(buffer)), 0);
#else
            const auto received = recv(connection, buffer, sizeof(buffer), 0);
#endif
            if (received == 0) {
                fail("The host closed the LAN session.");
                return;
            }
            if (received < 0) {
#ifdef _WIN32
                if (socketError() != WSAEWOULDBLOCK) {
#else
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
#endif
                    fail("The LAN connection was lost.");
                }
                return;
            }
            clientInput.append(buffer, static_cast<std::size_t>(received));
            if (clientInput.size() > kMaxLineLength * 32U) {
                fail("Received an oversized LAN message.");
                return;
            }
            std::size_t newline = clientInput.find('\n');
            while (newline != std::string::npos) {
                std::string line = clientInput.substr(0, newline);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                if (!line.empty() && !applyMessage(line, game, 0)) {
                    fail("Received an invalid LAN message.");
                    return;
                }
                clientInput.erase(0, newline + 1U);
                newline = clientInput.find('\n');
            }
        }
    }
};

LanSession::LanSession(std::unique_ptr<Impl> impl)
    : impl_(std::move(impl))
{
}

LanSession::~LanSession() = default;

std::unique_ptr<LanSession> LanSession::host(std::uint32_t seed,
                                             std::string &error,
                                             std::uint16_t port)
{
#ifdef _WIN32
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        error = "Could not initialize Windows networking.";
        return nullptr;
    }
#endif
    auto impl = std::make_unique<Impl>();
    impl->host = true;
    impl->seed = seed;
    impl->hostPort = port;
    impl->listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (impl->listener == kInvalidSocket) {
        error = "Could not create the LAN host socket (error " +
                std::to_string(socketError()) + ").";
        return nullptr;
    }
    int reuse = 1;
    setsockopt(impl->listener, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char *>(&reuse), sizeof(reuse));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    if (bind(impl->listener, reinterpret_cast<sockaddr *>(&address),
             sizeof(address)) != 0 ||
        listen(impl->listener, kMaxClients) != 0 ||
        !setNonblocking(impl->listener)) {
        error = "Could not listen on TCP port " +
                std::to_string(port) + " (error " +
                std::to_string(socketError()) + ").";
        return nullptr;
    }
    sockaddr_in boundAddress{};
#ifdef _WIN32
    int boundLength = sizeof(boundAddress);
#else
    socklen_t boundLength = sizeof(boundAddress);
#endif
    if (getsockname(impl->listener,
                    reinterpret_cast<sockaddr *>(&boundAddress),
                    &boundLength) != 0) {
        error = "Could not determine the LAN host port.";
        return nullptr;
    }
    impl->hostPort = ntohs(boundAddress.sin_port);
    return std::unique_ptr<LanSession>(new LanSession(std::move(impl)));
}

std::unique_ptr<LanSession> LanSession::join(const std::string &address,
                                             std::string &error)
{
#ifdef _WIN32
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        error = "Could not initialize Windows networking.";
        return nullptr;
    }
#endif
    std::string hostname = address;
    std::string port = std::to_string(kDefaultPort);
    const std::size_t colon = address.rfind(':');
    if (colon != std::string::npos &&
        address.find(':') == colon && colon + 1U < address.size()) {
        hostname = address.substr(0, colon);
        port = address.substr(colon + 1U);
    }
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo *results = nullptr;
    if (getaddrinfo(hostname.c_str(), port.c_str(), &hints, &results) != 0) {
        error = "Could not resolve the host address.";
        return nullptr;
    }
    Socket connection = kInvalidSocket;
    for (addrinfo *entry = results; entry != nullptr; entry = entry->ai_next) {
        connection = socket(entry->ai_family, entry->ai_socktype,
                            entry->ai_protocol);
        if (connection == kInvalidSocket) {
            continue;
        }
        if (!setNonblocking(connection)) {
            closeSocket(connection);
            connection = kInvalidSocket;
            continue;
        }
        const int result = connect(connection, entry->ai_addr,
                                   static_cast<int>(entry->ai_addrlen));
        if (result == 0) {
            break;
        }
#ifdef _WIN32
        if (socketError() != WSAEWOULDBLOCK &&
            socketError() != WSAEINPROGRESS) {
#else
        if (errno != EINPROGRESS) {
#endif
            closeSocket(connection);
            connection = kInvalidSocket;
            continue;
        }
        fd_set writable;
        FD_ZERO(&writable);
        FD_SET(connection, &writable);
        timeval timeout{5, 0};
#ifdef _WIN32
        const int ready = select(0, nullptr, &writable, nullptr, &timeout);
#else
        const int ready = select(connection + 1, nullptr, &writable, nullptr,
                                  &timeout);
#endif
        int connectError = 0;
#ifdef _WIN32
        int errorSize = sizeof(connectError);
#else
        socklen_t errorSize = sizeof(connectError);
#endif
        if (ready <= 0 ||
            getsockopt(connection, SOL_SOCKET, SO_ERROR,
                       reinterpret_cast<char *>(&connectError),
                       &errorSize) != 0 ||
            connectError != 0) {
            closeSocket(connection);
            connection = kInvalidSocket;
            continue;
        }
        break;
    }
    freeaddrinfo(results);
    if (connection == kInvalidSocket) {
        error = "Could not connect to " + address + ":" + port +
                ". Check the address, port, and firewall.";
        return nullptr;
    }

    const std::string hello = "HELLO 1\n";
#ifdef _WIN32
    send(connection, hello.c_str(), static_cast<int>(hello.size()), 0);
#else
    send(connection, hello.c_str(), hello.size(), MSG_NOSIGNAL);
#endif
    std::string welcome;
    if (!readHandshakeLine(connection, welcome)) {
        closeSocket(connection);
        error = "The host did not send a valid welcome message.";
        return nullptr;
    }
    std::istringstream response(welcome);
    std::string keyword;
    int version = 0;
    std::uint32_t seed = 0;
    int id = 0;
    std::string extra;
    if (!(response >> keyword >> version >> seed >> id) ||
        (response >> extra) || keyword != "WELCOME" || version != 1 ||
        id <= 1) {
        closeSocket(connection);
        error = "The host returned an incompatible Realms response.";
        return nullptr;
    }
    if (!setNonblocking(connection)) {
        closeSocket(connection);
        error = "Could not configure the LAN connection.";
        return nullptr;
    }
    auto impl = std::make_unique<Impl>();
    impl->connection = connection;
    impl->seed = seed;
    impl->localId = id;
    return std::unique_ptr<LanSession>(new LanSession(std::move(impl)));
}

std::uint32_t LanSession::worldSeed() const noexcept
{
    return impl_->seed;
}

bool LanSession::isHost() const noexcept
{
    return impl_->host;
}

std::uint16_t LanSession::port() const noexcept
{
    return impl_->hostPort;
}

void LanSession::pump(Game &game)
{
    if (impl_->failed) {
        return;
    }
    if (impl_->host) {
        impl_->acceptClients(game);
        for (Impl::Peer &peer : impl_->peers) {
            if (peer.socket != kInvalidSocket) {
                impl_->readPeerMessages(game, peer);
                if (peer.socket != kInvalidSocket) {
                    impl_->flush(peer.socket, peer.output);
                } else {
                    impl_->broadcast("D " + std::to_string(peer.id));
                }
            }
        }
        impl_->peers.erase(
            std::remove_if(impl_->peers.begin(), impl_->peers.end(),
                           [](const Impl::Peer &peer) {
                               return peer.socket == kInvalidSocket;
                           }),
            impl_->peers.end());
    } else {
        impl_->readClientMessages(game);
        if (!impl_->failed) {
            impl_->flush(impl_->connection, impl_->clientOutput);
        }
    }
}

void LanSession::publishBlock(int x, int y, int z, GcBlock block)
{
    if (impl_->failed) {
        return;
    }
    std::ostringstream message;
    message << "B " << x << ' ' << y << ' ' << z << ' '
            << static_cast<int>(block);
    if (impl_->host) {
        impl_->broadcast(message.str());
    } else {
        impl_->queueClient(message.str());
    }
}

void LanSession::publishPosition(const Game &game)
{
    if (impl_->failed) {
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    if (now - impl_->lastPosition < std::chrono::milliseconds(100)) {
        return;
    }
    impl_->lastPosition = now;
    const std::string message = positionMessage(impl_->localId, game);
    if (impl_->host) {
        impl_->broadcast(message);
    } else {
        std::ostringstream clientMessage;
        clientMessage.precision(6);
        clientMessage << "P " << impl_->localId << ' ' << game.playerX() << ' ' << game.playerY()
                      << ' ' << game.playerZ() << ' ' << game.yaw() << ' '
                      << game.pitch();
        impl_->queueClient(clientMessage.str());
    }
}

const std::string &LanSession::error() const noexcept
{
    return impl_->lastError;
}

}
