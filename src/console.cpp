#include "console.hpp"
#include "GameEngine/vehicles.hpp"
#include "glitchcraft/version.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace glitchcraft {
namespace {

constexpr float kFov = 1.10F;
const char *blockColor(GcBlock block)
{
    switch (block) {
    case GC_BLOCK_GRASS: return "\033[38;5;70m";
    case GC_BLOCK_DIRT: return "\033[38;5;130m";
    case GC_BLOCK_STONE: return "\033[38;5;245m";
    case GC_BLOCK_WOOD: return "\033[38;5;94m";
    case GC_BLOCK_WATER: return "\033[38;5;39m";
    case GC_BLOCK_LAVA: return "\033[38;5;202m";
    case GC_BLOCK_REDSTONE: return "\033[38;5;196m";
    case GC_BLOCK_SHULKER_WHITE: return "\033[38;5;255m";
    case GC_BLOCK_SHULKER_ORANGE: return "\033[38;5;208m";
    case GC_BLOCK_SHULKER_MAGENTA: return "\033[38;5;170m";
    case GC_BLOCK_SHULKER_LIGHT_BLUE: return "\033[38;5;117m";
    case GC_BLOCK_SHULKER_YELLOW: return "\033[38;5;226m";
    case GC_BLOCK_SHULKER_LIME: return "\033[38;5;118m";
    case GC_BLOCK_SHULKER_PINK: return "\033[38;5;211m";
    case GC_BLOCK_SHULKER_GRAY: return "\033[38;5;244m";
    case GC_BLOCK_SHULKER_LIGHT_GRAY: return "\033[38;5;250m";
    case GC_BLOCK_SHULKER_CYAN: return "\033[38;5;44m";
    case GC_BLOCK_SHULKER_PURPLE: return "\033[38;5;135m";
    case GC_BLOCK_SHULKER_BLUE: return "\033[38;5;63m";
    case GC_BLOCK_SHULKER_BROWN: return "\033[38;5;130m";
    case GC_BLOCK_SHULKER_GREEN: return "\033[38;5;64m";
    case GC_BLOCK_SHULKER_RED: return "\033[38;5;160m";
    case GC_BLOCK_SHULKER_BLACK: return "\033[38;5;232m";
    case GC_BLOCK_ASPHALT:
    case GC_BLOCK_ROAD_STRAIGHT:
    case GC_BLOCK_ROAD_CURVE:
    case GC_BLOCK_ROAD_T_INTERSECTION:
    case GC_BLOCK_ROAD_CROSS_INTERSECTION:
    case GC_BLOCK_ROAD_DEAD_END:
    case GC_BLOCK_ROAD_ROUNDABOUT:
        return "\033[38;5;240m";
    default: return "\033[0m";
    }
}

char blockGlyph(GcBlock block, float shade)
{
    if (block == GC_BLOCK_WATER) return '~';
    if (block == GC_BLOCK_LAVA) return '^';
    if (block == GC_BLOCK_REDSTONE) return '*';
    if (shade > 0.72F) return '#';
    if (shade > 0.43F) return '+';
    return '.';
}

}

Console::Console()
    : width_(80), height_(24), terminalMode_(interactive()), rawMode_(false),
      originalFlags_(-1)
{
#ifndef _WIN32
    if (terminalMode_) {
        struct winsize size {};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0) {
            width_ = std::max(40, static_cast<int>(size.ws_col));
            height_ = std::max(18, static_cast<int>(size.ws_row));
        }
        struct termios mode {};
        if (tcgetattr(STDIN_FILENO, &mode) == 0) {
            struct termios raw = mode;
            raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            rawMode_ = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0;
            originalFlags_ = fcntl(STDIN_FILENO, F_GETFL, 0);
            if (originalFlags_ >= 0) {
                fcntl(STDIN_FILENO, F_SETFL, originalFlags_ | O_NONBLOCK);
            }
        }
        std::cout << "\033[?1049h\033[?25l";
    }
#endif
}

Console::~Console()
{
#ifndef _WIN32
    if (terminalMode_) {
        if (rawMode_) {
            struct termios mode {};
            tcgetattr(STDIN_FILENO, &mode);
            mode.c_lflag |= static_cast<tcflag_t>(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &mode);
        }
        if (originalFlags_ >= 0) {
            fcntl(STDIN_FILENO, F_SETFL, originalFlags_);
        }
        std::cout << "\033[0m\033[?25h\033[?1049l" << std::flush;
    }
#endif
}

int Console::width() const noexcept { return width_; }
int Console::height() const noexcept { return height_; }

bool Console::interactive() noexcept
{
#ifndef _WIN32
    return isatty(STDIN_FILENO) != 0 && isatty(STDOUT_FILENO) != 0;
#else
    return false;
#endif
}

Key Console::readKey(bool inventoryMode)
{
    if (!terminalMode_) {
        return Key::None;
    }
#ifndef _WIN32
    unsigned char key = 0;
    if (read(STDIN_FILENO, &key, 1) != 1) {
        return Key::None;
    }
    if (inventoryMode) {
        switch (key) {
        case 'g': case 'G': return Key::SelectGrass;
        case 'd': case 'D': return Key::SelectDirt;
        case 's': case 'S': return Key::SelectStone;
        case 'w': case 'W': return Key::SelectWood;
        case 't': case 'T': return Key::SelectRedstone;
        case 'u': case 'U': return Key::SelectWater;
        case 'v': case 'V': return Key::SelectLava;
        case '[': return Key::SelectPreviousBuildItem;
        case ']': return Key::SelectNextBuildItem;
        default: break;
        }
    }
    switch (key) {
    case 'w': case 'W': return Key::Forward;
    case 's': case 'S': return Key::Backward;
    case 'a': case 'A': return Key::StrafeLeft;
    case 'd': case 'D': return Key::StrafeRight;
    case 'i': case 'I': return Key::Up;
    case 'k': case 'K': return Key::Down;
    case 'j': case 'J': return Key::Left;
    case 'l': case 'L': return Key::Right;
    case ' ': return Key::Jump;
    case 'f': case 'F': return Key::ToggleFlight;
    case 'r': case 'R': return Key::Mine;
    case 'p': case 'P': return Key::Place;
    case 'x': case 'X': return Key::Inventory;
    case 'b': case 'B': return Key::CloseInventory;
    case '1': return Key::Potion;
    case '2': return Key::Milk;
    case '3': return Key::EatCake;
    case '4': return Key::EatCookie;
    case '5': return Key::EatCrackers;
    case '6': return Key::EatSandwich;
    case '7': return Key::EatSpaghetti;
    case '8': return Key::EatPizza;
    case '9': return Key::EatCottonCandy;
    case 't': case 'T': return Key::SelectRedstone;
    case 'u': case 'U': return Key::SelectWater;
    case 'v': case 'V': return Key::SelectLava;
    case 'y': case 'Y': return Key::Confirm;
    case 'c': case 'C': return Key::CycleBuildMode;
    case '[': return Key::SelectPreviousBuildItem;
    case ']': return Key::SelectNextBuildItem;
    case 'o': case 'O': return Key::RotateTrack;
    case 'e': case 'E': return Key::RefuelTrain;
    case 'h': case 'H': return Key::ToggleTrackSwitch;
    case 'n': case 'N': return Key::Back;
    case '\r': case '\n': return Key::Confirm;
    case 27: {
        unsigned char sequence[2]{};
        if (read(STDIN_FILENO, &sequence[0], 1) != 1 ||
            sequence[0] != '[' ||
            read(STDIN_FILENO, &sequence[1], 1) != 1) {
            return Key::Pause;
        }
        switch (sequence[1]) {
        case 'A': return Key::Up;
        case 'B': return Key::Down;
        case 'C': return Key::Right;
        case 'D': return Key::Left;
        default: return Key::None;
        }
    }
    case 'q': case 'Q': return Key::Quit;
    case 'm': case 'M': return Key::Realms;
    default: return Key::None;
    }
#else
    return Key::None;
#endif
}

std::string Console::readText(const std::string &prompt)
{
    std::cout << "\033[0m" << prompt << std::flush;
    std::string text;
    if (!terminalMode_) {
        std::getline(std::cin, text);
        return text;
    }
#ifndef _WIN32
    for (;;) {
        unsigned char character = 0;
        if (read(STDIN_FILENO, &character, 1) != 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (character == '\r' || character == '\n') {
            std::cout << '\n' << std::flush;
            return text;
        }
        if (character == 27) {
            std::cout << '\n' << std::flush;
            return {};
        }
        if (character == 8 || character == 127) {
            if (!text.empty()) {
                text.pop_back();
                std::cout << "\b \b" << std::flush;
            }
        } else if (character >= 32 && character <= 126 &&
                   text.size() < 128U) {
            text.push_back(static_cast<char>(character));
            std::cout << static_cast<char>(character) << std::flush;
        }
    }
#else
    std::getline(std::cin, text);
    return text;
#endif
}

void Console::clear()
{
    if (terminalMode_) {
        std::cout << "\033[2J\033[H";
    } else {
        std::cout << '\n';
    }
}

void Console::drawTextScreen(const std::string &title,
                             const std::string &body,
                             const std::string &footer)
{
    clear();
    std::cout << "============================================================\n"
              << "       GLITCHCRAFT: GAMECUBE EDITION - "
              << glitchcraft::kReleaseStage << " "
              << glitchcraft::kVersion << "\n"
              << "============================================================\n\n"
              << title << "\n\n" << body << "\n\n" << footer << '\n';
    std::cout.flush();
}

void Console::drawTitle()
{
    drawTextScreen("A block-built world awaits.",
                   "  [Enter]  Singleplayer\n"
                   "  [M]      GlitchCraft Realms (LAN host/join)\n"
                   "  [Q]      Quit\n\n"
                   "Stable release 1.0.0: the listed game systems are available.\n"
                   "An original voxel sandbox; not affiliated with Mojang.",
                   "Choose an option.");
}

void Console::drawTerms()
{
    drawTextScreen("Terms of Service",
                   "This release is provided as-is. Worlds are stored only\n"
                   "in memory. Optional LAN sessions connect directly to a host;\n"
                   "there are no accounts, cloud saves, or voice chat.\n"
                   "Accept to continue.",
                   "[Y] Accept and remember   [N] Decline and quit");
}

void Console::drawWorldSelect()
{
    drawTextScreen("Create a world",
                   "  [1]  Start a new singleplayer world\n"
                   "  [2]  Return to title\n\n"
                   "Terrain is generated deterministically as you explore.",
                   "Choose an option.");
}

void Console::drawRealms()
{
    drawTextScreen("GlitchCraft Realms",
                   "Private multiplayer hosted on your local network.\n"
                   "Host a world or join using the host's IPv4 address.\n"
                   "TCP port 28777 must be reachable on your LAN.\n"
                   "This game does not use accounts or cloud servers.",
                   "[H] Host a world   [J] Join a world   [Enter] Return");
}

void Console::drawInventory(GcBlock selected, int foodLevel,
                            GcTrackType selectedTrack,
                            GcVehicleType selectedVehicle, int buildMode)
{
    const char *selectedName = "Grass";
    switch (selected) {
    case GC_BLOCK_DIRT: selectedName = "Dirt"; break;
    case GC_BLOCK_STONE: selectedName = "Stone"; break;
    case GC_BLOCK_WOOD: selectedName = "Wood"; break;
    case GC_BLOCK_REDSTONE: selectedName = "Redstone"; break;
    case GC_BLOCK_WATER: selectedName = "Water"; break;
    case GC_BLOCK_LAVA: selectedName = "Lava"; break;
    case GC_BLOCK_SHULKER_WHITE: selectedName = "White Shulker Box"; break;
    case GC_BLOCK_SHULKER_ORANGE: selectedName = "Orange Shulker Box"; break;
    case GC_BLOCK_SHULKER_MAGENTA: selectedName = "Magenta Shulker Box"; break;
    case GC_BLOCK_SHULKER_LIGHT_BLUE: selectedName = "Light Blue Shulker Box"; break;
    case GC_BLOCK_SHULKER_YELLOW: selectedName = "Yellow Shulker Box"; break;
    case GC_BLOCK_SHULKER_LIME: selectedName = "Lime Shulker Box"; break;
    case GC_BLOCK_SHULKER_PINK: selectedName = "Pink Shulker Box"; break;
    case GC_BLOCK_SHULKER_GRAY: selectedName = "Gray Shulker Box"; break;
    case GC_BLOCK_SHULKER_LIGHT_GRAY: selectedName = "Light Gray Shulker Box"; break;
    case GC_BLOCK_SHULKER_CYAN: selectedName = "Cyan Shulker Box"; break;
    case GC_BLOCK_SHULKER_PURPLE: selectedName = "Purple Shulker Box"; break;
    case GC_BLOCK_SHULKER_BLUE: selectedName = "Blue Shulker Box"; break;
    case GC_BLOCK_SHULKER_BROWN: selectedName = "Brown Shulker Box"; break;
    case GC_BLOCK_SHULKER_GREEN: selectedName = "Green Shulker Box"; break;
    case GC_BLOCK_SHULKER_RED: selectedName = "Red Shulker Box"; break;
    case GC_BLOCK_SHULKER_BLACK: selectedName = "Black Shulker Box"; break;
    case GC_BLOCK_ASPHALT: selectedName = "Asphalt"; break;
    case GC_BLOCK_ROAD_STRAIGHT: selectedName = "Straight Road"; break;
    case GC_BLOCK_ROAD_CURVE: selectedName = "Curved Road"; break;
    case GC_BLOCK_ROAD_T_INTERSECTION: selectedName = "T-Intersection Road"; break;
    case GC_BLOCK_ROAD_CROSS_INTERSECTION:
        selectedName = "Cross-Intersection Road";
        break;
    case GC_BLOCK_ROAD_DEAD_END: selectedName = "Dead-End Road"; break;
    case GC_BLOCK_ROAD_ROUNDABOUT: selectedName = "Roundabout Road"; break;
    default: break;
    }
    const std::string selectedItem = buildMode == 2
        ? "Selected vehicle: " + gameengine::vehicleName(selectedVehicle)
        : buildMode == 1
            ? "Selected track: " + std::to_string(selectedTrack)
            : "Selected block: " + std::string(selectedName);
    drawTextScreen("Inventory",
                   selectedItem + "  |  Food: " +
                       std::to_string(foodLevel) + "/100\n"
                       "Blocks: G grass, D dirt, S stone, W wood, T redstone,\n"
                       "        U water, V lava (not drinkable)\n"
                       "Cycle selected block with [ / ], including all 16 shulker colors.\n"
                       "Items: 1 potion, 2 milk\n"
                       "Food: 3 cake, 4 cookie, 5 crackers, 6 sandwich,\n"
                       "      7 spaghetti, 8 pizza, 9 cotton candy",
                   "[B] or [X] Close");
}

void Console::drawShulker(GcBlock selected, int activeSlot,
                          const ShulkerStack &stack)
{
    static const char *blockNames[]{
        "Air", "Grass", "Dirt", "Stone", "Wood", "Water", "Lava",
        "Redstone", "White Shulker", "Orange Shulker", "Magenta Shulker",
        "Light Blue Shulker", "Yellow Shulker", "Lime Shulker",
        "Pink Shulker", "Gray Shulker", "Light Gray Shulker",
        "Cyan Shulker", "Purple Shulker", "Blue Shulker", "Brown Shulker",
        "Green Shulker", "Red Shulker", "Black Shulker", "Asphalt",
        "Straight Road", "Curved Road", "T-Intersection Road",
        "Cross-Intersection Road", "Dead-End Road", "Roundabout Road"};
    const int selectedIndex = static_cast<int>(selected);
    const int stackIndex = static_cast<int>(stack.block);
    const std::string item = stack.count > 0 && stackIndex >= 0 &&
            stackIndex < static_cast<int>(sizeof(blockNames) /
                                         sizeof(blockNames[0]))
        ? blockNames[stackIndex] + std::string(" x") +
              std::to_string(stack.count)
        : "Empty";
    drawTextScreen(
        "Shulker Box",
        "Slot " + std::to_string(activeSlot + 1) + "/27: " + item + "\n"
        "Selected item: " +
            (selectedIndex >= 0 &&
                     selectedIndex < static_cast<int>(sizeof(blockNames) /
                                                      sizeof(blockNames[0]))
                 ? std::string(blockNames[selectedIndex])
                 : std::string("Unknown")) +
            "\n[ / ] choose slot   Y store 64 selected blocks   N clear slot\n"
            "X or B close. Shulker contents are kept in this world session.",
        "Container");
}

void Console::drawPaused()
{
    drawTextScreen("Paused", "Your world is waiting.", "[Esc] Resume   [Q] Quit");
}

void Console::drawMessage(const std::string &message)
{
    drawTextScreen("GlitchCraft", message, "[Enter] Continue");
}

void Console::drawGame(const Game &game)
{
    const int columns = std::clamp(width_ - 2, 38, 120);
    const int rows = std::clamp(height_ - 5, 12, 38);
    std::vector<std::string> frame(static_cast<std::size_t>(rows),
                                   std::string(static_cast<std::size_t>(columns), ' '));
    std::vector<std::vector<std::string>> colors(
        static_cast<std::size_t>(rows),
        std::vector<std::string>(static_cast<std::size_t>(columns), "\033[0m"));
    for (int column = 0; column < columns; ++column) {
        const float camera = (static_cast<float>(column) /
                              static_cast<float>(columns - 1) - 0.5F) * kFov;
        const float rayAngle = game.yaw() + camera;
        const auto hit = game.castRay(rayAngle, game.pitch(), 24.0F);
        const float correctedDistance = hit.hit
            ? std::max(0.12F, hit.distance * std::cos(camera))
            : 24.0F;
        const int horizon = rows / 2 -
            static_cast<int>(game.pitch() * static_cast<float>(rows) * 0.55F);
        const int wallHeight = hit.hit
            ? std::min(rows * 3, static_cast<int>(
                  static_cast<float>(rows) * 1.15F / correctedDistance))
            : 0;
        const int top = horizon - wallHeight / 2;
        const int bottom = horizon + wallHeight / 2;

        for (int row = 0; row < rows; ++row) {
            if (hit.hit && row >= top && row <= bottom) {
                const float shade = std::clamp(1.0F - correctedDistance / 22.0F -
                                                    hit.side,
                                                0.12F, 1.0F);
                frame[static_cast<std::size_t>(row)]
                     [static_cast<std::size_t>(column)] = blockGlyph(hit.block, shade);
                colors[static_cast<std::size_t>(row)]
                      [static_cast<std::size_t>(column)] = blockColor(hit.block);
            } else if (row < horizon) {
                frame[static_cast<std::size_t>(row)]
                     [static_cast<std::size_t>(column)] =
                         row < horizon / 2 ? ' ' : '.';
            } else {
                frame[static_cast<std::size_t>(row)]
                     [static_cast<std::size_t>(column)] =
                         ((row + column / 7) % 3 == 0) ? ':' : '.';
            }
        }
    }

    if (columns > 2 && rows > 2) {
        frame[static_cast<std::size_t>(rows / 2)]
             [static_cast<std::size_t>(columns / 2)] = '+';
    }
    std::cout << "\033[H";
    for (int row = 0; row < rows; ++row) {
        std::string activeColor;
        for (int column = 0; column < columns; ++column) {
            const auto &color = colors[static_cast<std::size_t>(row)]
                                       [static_cast<std::size_t>(column)];
            if (color != activeColor) {
                std::cout << color;
                activeColor = color;
            }
            std::cout << frame[static_cast<std::size_t>(row)]
                              [static_cast<std::size_t>(column)];
        }
        std::cout << "\033[0m\033[K\n";
    }
    std::cout << "\033[0mWASD move  IJKL look  Space jump  F fly  R mine  P place\n"
              << "X inventory  C build mode  [ ] select  O rotate  Y power\n"
              << "E refuel  H switch  Rails: " << game.tracks().size()
              << "  Vehicles: " << game.vehicles().size()
              << "  Esc pause  Q quit";
    if (game.potionTicks() > 0) {
        std::cout << "  Potion: " << game.potionTicks() / 60 << "s";
    }
    if (game.flying()) {
        std::cout << "  Flying";
    }
    std::cout << "\033[K" << std::flush;
}

}
