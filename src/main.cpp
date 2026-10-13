#include "console.hpp"
#include "engine.hpp"
#include "GameEngine/renderer.hpp"
#include "GameEngine/window.hpp"
#include "realms.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

namespace {

constexpr const char *kTermsFile = ".glitchcraft_terms_accepted";

std::filesystem::path termsPath()
{
    const char *home = std::getenv("HOME");
    return (home == nullptr ? std::filesystem::current_path()
                            : std::filesystem::path(home)) / kTermsFile;
}

bool termsAccepted()
{
    std::ifstream file(termsPath());
    std::string accepted;
    return file >> accepted && accepted == "accepted";
}

bool rememberTermsAcceptance()
{
    std::ofstream file(termsPath(), std::ios::trunc);
    if (!file) {
        return false;
    }
    file << "accepted\n";
    return file.good();
}

void waitForKey(glitchcraft::Console &console)
{
    while (console.readKey() == glitchcraft::Key::None) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void runGame(glitchcraft::Console &console, std::uint32_t seed,
             glitchcraft::LanSession *session = nullptr)
{
    glitchcraft::Game game(session == nullptr ? seed : session->worldSeed());
    glitchcraft::gameengine::Window window;
    const bool graphicsEnabled = window.open(960, 540);
    glitchcraft::gameengine::Renderer renderer(320, 180);
    GcBlock selectedBlock = GC_BLOCK_GRASS;
    GcTrackType selectedTrack = GC_TRACK_STRAIGHT;
    GcVehicleType selectedVehicle = GC_VEHICLE_BIG_BOY;
    bool trackAxisX = true;
    int buildMode = 0;
    bool inventoryOpen = false;
    bool paused = false;
    bool running = true;
    glitchcraft::Key lastMovementKey = glitchcraft::Key::None;
    auto lastMovementTime = std::chrono::steady_clock::time_point{};
    auto previous = std::chrono::steady_clock::now();
    auto movePlayer = [&](glitchcraft::Key movementKey, float forward, float strafe) {
        const auto now = std::chrono::steady_clock::now();
        const bool sprint = movementKey == lastMovementKey &&
            now - lastMovementTime <= std::chrono::milliseconds(350);
        game.move(forward, strafe, sprint);
        lastMovementKey = movementKey;
        lastMovementTime = now;
    };
    auto placeSelected = [&]() {
        if (buildMode == 1) {
            game.placeTrack(selectedTrack, trackAxisX);
        } else if (buildMode == 2) {
            game.placeVehicle(selectedVehicle);
        } else {
            const auto hit = game.castRay(game.yaw(), game.pitch(), 8.0F);
            if (game.placeBlock(selectedBlock) && session != nullptr && hit.hit) {
                session->publishBlock(hit.adjacent.x, hit.adjacent.y,
                                      hit.adjacent.z, selectedBlock);
            }
        }
    };
    auto mineTarget = [&]() {
        const auto hit = game.castRay(game.yaw(), game.pitch(), 8.0F);
        if (game.mineBlock() && session != nullptr && hit.hit && !hit.track) {
            session->publishBlock(hit.position.x, hit.position.y,
                                  hit.position.z, GC_BLOCK_AIR);
        }
    };

    while (running && (!graphicsEnabled || window.isOpen())) {
        if (session != nullptr) {
            session->pump(game);
            if (!session->error().empty()) {
                break;
            }
            session->publishPosition(game);
        }
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = now - previous;
        previous = now;

        glitchcraft::Key key = console.readKey(inventoryOpen);
        int windowKey = 0;
        if (graphicsEnabled && window.pollKey(windowKey)) {
            if (inventoryOpen) {
                if (windowKey == 'g' || windowKey == 'G') {
                    key = glitchcraft::Key::SelectGrass;
                } else if (windowKey == 'd' || windowKey == 'D') {
                    key = glitchcraft::Key::SelectDirt;
                } else if (windowKey == 's' || windowKey == 'S') {
                    key = glitchcraft::Key::SelectStone;
                } else if (windowKey == 'w' || windowKey == 'W') {
                    key = glitchcraft::Key::SelectWood;
                }
            }
            if (key != glitchcraft::Key::SelectDirt &&
                key != glitchcraft::Key::SelectStone &&
                key != glitchcraft::Key::SelectWood) {
                switch (windowKey) {
                case 0x101: key = glitchcraft::Key::Left; break;
                case 0x102: key = glitchcraft::Key::Right; break;
                case 0x103: key = glitchcraft::Key::Up; break;
                case 0x104: key = glitchcraft::Key::Down; break;
                case 27: key = glitchcraft::Key::Pause; break;
                case 'w': case 'W': key = glitchcraft::Key::Forward; break;
                case 's': case 'S': key = glitchcraft::Key::Backward; break;
                case 'a': case 'A': key = glitchcraft::Key::StrafeLeft; break;
                case 'd': case 'D': key = glitchcraft::Key::StrafeRight; break;
                case ' ': key = glitchcraft::Key::Jump; break;
                case 'f': case 'F': key = glitchcraft::Key::ToggleFlight; break;
                case 'r': case 'R': key = glitchcraft::Key::Mine; break;
                case 'p': case 'P': key = glitchcraft::Key::Place; break;
                case 'x': case 'X':
                    key = inventoryOpen ? glitchcraft::Key::CloseInventory
                                        : glitchcraft::Key::Inventory;
                    break;
                case 'b': case 'B': key = glitchcraft::Key::CloseInventory; break;
                case '1': key = glitchcraft::Key::Potion; break;
                case '2': key = glitchcraft::Key::Milk; break;
                case '3': key = glitchcraft::Key::EatCake; break;
                case '4': key = glitchcraft::Key::EatCookie; break;
                case '5': key = glitchcraft::Key::EatCrackers; break;
                case '6': key = glitchcraft::Key::EatSandwich; break;
                case '7': key = glitchcraft::Key::EatSpaghetti; break;
                case '8': key = glitchcraft::Key::EatPizza; break;
                case '9': key = glitchcraft::Key::EatCottonCandy; break;
                case 'g': case 'G': key = glitchcraft::Key::SelectGrass; break;
                case 't': case 'T': key = glitchcraft::Key::SelectRedstone; break;
                case 'u': case 'U': key = glitchcraft::Key::SelectWater; break;
                case 'v': case 'V': key = glitchcraft::Key::SelectLava; break;
                case 'c': case 'C':
                    key = glitchcraft::Key::CycleBuildMode;
                    break;
                case '[': key = glitchcraft::Key::SelectPreviousBuildItem; break;
                case ']': key = glitchcraft::Key::SelectNextBuildItem; break;
                case 'o': case 'O': key = glitchcraft::Key::RotateTrack; break;
                case 'e': case 'E': key = glitchcraft::Key::RefuelTrain; break;
                case 'h': case 'H': key = glitchcraft::Key::ToggleTrackSwitch; break;
                case 'y': case 'Y': key = glitchcraft::Key::Confirm; break;
                case 'q': case 'Q': key = glitchcraft::Key::Quit; break;
                default: key = glitchcraft::Key::None; break;
                }
            }
        }
        if (paused) {
            if (key == glitchcraft::Key::Pause || key == glitchcraft::Key::Confirm) {
                paused = false;
                game.setPaused(false);
            } else if (key == glitchcraft::Key::Quit) {
                running = false;
            }
            if (graphicsEnabled) {
                window.drawOverlay("Paused", {"Esc resume    Q quit"});
            } else {
                console.drawPaused();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(60));
            continue;
        }

        if (inventoryOpen) {
            if (key == glitchcraft::Key::Pause) {
                inventoryOpen = false;
                paused = true;
                game.setPaused(true);
            } else if (key == glitchcraft::Key::Potion) {
                game.drinkPotion();
            } else if (key == glitchcraft::Key::Milk) {
                game.drinkMilk();
            } else if (key >= glitchcraft::Key::EatCake &&
                       key <= glitchcraft::Key::EatCottonCandy) {
                const auto food = static_cast<GcFood>(
                    static_cast<int>(key) -
                    static_cast<int>(glitchcraft::Key::EatCake));
                game.eatFood(food);
            } else if (key == glitchcraft::Key::SelectGrass) {
                selectedBlock = GC_BLOCK_GRASS;
            } else if (key == glitchcraft::Key::SelectDirt) {
                selectedBlock = GC_BLOCK_DIRT;
            } else if (key == glitchcraft::Key::SelectStone) {
                selectedBlock = GC_BLOCK_STONE;
            } else if (key == glitchcraft::Key::SelectWood) {
                selectedBlock = GC_BLOCK_WOOD;
            } else if (key == glitchcraft::Key::SelectRedstone) {
                selectedBlock = GC_BLOCK_REDSTONE;
            } else if (key == glitchcraft::Key::SelectWater) {
                selectedBlock = GC_BLOCK_WATER;
            } else if (key == glitchcraft::Key::SelectLava) {
                selectedBlock = GC_BLOCK_LAVA;
            } else if (key == glitchcraft::Key::CycleBuildMode) {
                buildMode = (buildMode + 1) % 3;
            } else if (key == glitchcraft::Key::SelectNextBuildItem) {
                if (buildMode == 1) {
                    selectedTrack = static_cast<GcTrackType>(
                        (static_cast<int>(selectedTrack) + 1) % GC_TRACK_COUNT);
                } else if (buildMode == 2) {
                    selectedVehicle = static_cast<GcVehicleType>(
                        (static_cast<int>(selectedVehicle) + 1) %
                        GC_VEHICLE_COUNT);
                }
            } else if (key == glitchcraft::Key::SelectPreviousBuildItem) {
                if (buildMode == 1) {
                    selectedTrack = static_cast<GcTrackType>(
                        (static_cast<int>(selectedTrack) + GC_TRACK_COUNT - 1) %
                        GC_TRACK_COUNT);
                } else if (buildMode == 2) {
                    selectedVehicle = static_cast<GcVehicleType>(
                        (static_cast<int>(selectedVehicle) +
                         GC_VEHICLE_COUNT - 1) % GC_VEHICLE_COUNT);
                }
            } else if (key == glitchcraft::Key::RotateTrack) {
                trackAxisX = !trackAxisX;
            } else if (key == glitchcraft::Key::Confirm) {
                game.toggleTrainPower();
            } else if (key == glitchcraft::Key::RefuelTrain) {
                game.addTrainFuel(100);
            } else if (key == glitchcraft::Key::ToggleTrackSwitch) {
                game.toggleTrackSwitch();
            } else if (key == glitchcraft::Key::CloseInventory ||
                       key == glitchcraft::Key::Inventory ||
                       key == glitchcraft::Key::Back) {
                inventoryOpen = false;
            }
            if (graphicsEnabled) {
                window.drawOverlay(
                    "Inventory",
                        {"Build: " + std::string(
                             buildMode == 0 ? "blocks" :
                             buildMode == 1 ? "tracks" : "rolling stock") +
                         "   Food: " + std::to_string(game.foodLevel()) + "/100",
                         "G grass  D dirt  S stone  W wood  T redstone",
                         "U water  V lava  1 potion  2 milk",
                         "3 cake  4 cookie  5 crackers  6 sandwich",
                         "7 spaghetti  8 pizza  9 cotton candy",
                         "[ / ] select rail or vehicle  O rotate rail",
                         "E refuel targeted engine/tender  H toggle switch",
                         "X or B close inventory"});
            } else {
                console.drawInventory(selectedBlock, game.foodLevel());
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(60));
            continue;
        }

        switch (key) {
        case glitchcraft::Key::Up: game.look(0.0F, 0.08F); break;
        case glitchcraft::Key::Down: game.look(0.0F, -0.08F); break;
        case glitchcraft::Key::Left: game.look(-0.08F, 0.0F); break;
        case glitchcraft::Key::Right: game.look(0.08F, 0.0F); break;
        case glitchcraft::Key::Forward: movePlayer(key, 1.0F, 0.0F); break;
        case glitchcraft::Key::Backward: movePlayer(key, -1.0F, 0.0F); break;
        case glitchcraft::Key::StrafeLeft: movePlayer(key, 0.0F, -1.0F); break;
        case glitchcraft::Key::StrafeRight: movePlayer(key, 0.0F, 1.0F); break;
        case glitchcraft::Key::Jump: game.jump(); break;
        case glitchcraft::Key::ToggleFlight: game.setFlying(!game.flying()); break;
        case glitchcraft::Key::Mine: mineTarget(); break;
        case glitchcraft::Key::Place: placeSelected(); break;
        case glitchcraft::Key::Inventory: inventoryOpen = true; break;
        case glitchcraft::Key::Potion: game.drinkPotion(); break;
        case glitchcraft::Key::Milk: game.drinkMilk(); break;
        case glitchcraft::Key::CycleBuildMode:
            buildMode = (buildMode + 1) % 3;
            break;
        case glitchcraft::Key::SelectNextBuildItem:
            if (buildMode == 1) {
                selectedTrack = static_cast<GcTrackType>(
                    (static_cast<int>(selectedTrack) + 1) % GC_TRACK_COUNT);
            } else if (buildMode == 2) {
                selectedVehicle = static_cast<GcVehicleType>(
                    (static_cast<int>(selectedVehicle) + 1) %
                    GC_VEHICLE_COUNT);
            }
            break;
        case glitchcraft::Key::SelectPreviousBuildItem:
            if (buildMode == 1) {
                selectedTrack = static_cast<GcTrackType>(
                    (static_cast<int>(selectedTrack) + GC_TRACK_COUNT - 1) %
                    GC_TRACK_COUNT);
            } else if (buildMode == 2) {
                selectedVehicle = static_cast<GcVehicleType>(
                    (static_cast<int>(selectedVehicle) +
                     GC_VEHICLE_COUNT - 1) % GC_VEHICLE_COUNT);
            }
            break;
        case glitchcraft::Key::RotateTrack: trackAxisX = !trackAxisX; break;
        case glitchcraft::Key::Confirm: game.toggleTrainPower(); break;
        case glitchcraft::Key::RefuelTrain: game.addTrainFuel(100); break;
        case glitchcraft::Key::ToggleTrackSwitch: game.toggleTrackSwitch(); break;
        case glitchcraft::Key::Pause: game.setPaused(true); paused = true; break;
        case glitchcraft::Key::Quit: running = false; break;
        default: break;
        }

        const auto ticks = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() /
                           16;
        for (std::int64_t tick = 0; tick < ticks; ++tick) {
            game.tick();
        }
        if (graphicsEnabled) {
            renderer.render(game);
            window.present(renderer, game);
        } else {
            console.drawGame(game);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    if (session != nullptr && !session->error().empty()) {
        console.drawMessage(session->error());
        waitForKey(console);
    }
}

}

int main()
{
    glitchcraft::Console console;
    if (!glitchcraft::Console::interactive()) {
        std::cerr << "GlitchCraft requires an interactive terminal.\n";
        return 1;
    }
    if (!termsAccepted()) {
        console.drawTerms();
        for (;;) {
            const auto key = console.readKey();
            if (key == glitchcraft::Key::Forward || key == glitchcraft::Key::Confirm) {
                if (!rememberTermsAcceptance()) {
                    console.drawMessage("Could not save Terms of Service acceptance.");
                    waitForKey(console);
                    return 1;
                }
                break;
            }
            if (key == glitchcraft::Key::Backward || key == glitchcraft::Key::Back ||
                key == glitchcraft::Key::Quit) {
                return 0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
        }
    }

    bool atTitle = true;
    while (atTitle) {
        console.drawTitle();
        const auto key = console.readKey();
        if (key == glitchcraft::Key::Quit) {
            break;
        }
        if (key == glitchcraft::Key::Realms) {
            console.drawRealms();
            std::string choice = console.readText("Choose H, J, or Enter: ");
            std::transform(choice.begin(), choice.end(), choice.begin(),
                           [](unsigned char character) {
                               return static_cast<char>(std::tolower(character));
                           });
            if (choice == "h" || choice == "host") {
                const auto seed = static_cast<std::uint32_t>(
                    std::chrono::system_clock::now().time_since_epoch().count());
                std::string error;
                auto session = glitchcraft::LanSession::host(seed, error);
                if (!session) {
                    console.drawMessage(error);
                    waitForKey(console);
                } else {
                    console.drawMessage(
                        "Hosting GlitchCraft Realms on TCP port 28777.\n"
                        "Joiners should connect to this computer's LAN IPv4 "
                        "address.\n"
                        "Press any key to enter the shared world.");
                    waitForKey(console);
                    runGame(console, seed, session.get());
                }
            } else if (choice == "j" || choice == "join") {
                const std::string address = console.readText(
                    "Host address (IPv4 or hostname, optional :port): ");
                if (!address.empty()) {
                    std::string error;
                    auto session =
                        glitchcraft::LanSession::join(address, error);
                    if (!session) {
                        console.drawMessage(error);
                        waitForKey(console);
                    } else {
                        runGame(console, session->worldSeed(), session.get());
                    }
                }
            }
        } else if (key == glitchcraft::Key::Confirm) {
            console.drawWorldSelect();
            for (;;) {
                const auto choice = console.readKey();
                if (choice == glitchcraft::Key::Potion ||
                    choice == glitchcraft::Key::Confirm) {
                    const auto seed = static_cast<std::uint32_t>(
                        std::chrono::system_clock::now().time_since_epoch().count());
                    runGame(console, seed);
                    break;
                }
                if (choice == glitchcraft::Key::Milk ||
                    choice == glitchcraft::Key::Back) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(40));
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
    return 0;
}
