#ifndef GLITCHCRAFT_REALMS_HPP
#define GLITCHCRAFT_REALMS_HPP

#include "engine.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace glitchcraft {

class LanSession {
public:
    static constexpr std::uint16_t kDefaultPort = 28777;

    ~LanSession();
    LanSession(const LanSession &) = delete;
    LanSession &operator=(const LanSession &) = delete;

    static std::unique_ptr<LanSession> host(
        std::uint32_t seed, std::string &error,
        std::uint16_t port = kDefaultPort);
    static std::unique_ptr<LanSession> join(const std::string &address,
                                            std::string &error);

    std::uint32_t worldSeed() const noexcept;
    bool isHost() const noexcept;
    std::uint16_t port() const noexcept;
    void pump(Game &game);
    void publishBlock(int x, int y, int z, GcBlock block);
    void publishPosition(const Game &game);
    const std::string &error() const noexcept;

private:
    struct Impl;
    explicit LanSession(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl_;
};

}

#endif
