#ifndef GLITCHCRAFT_CONSOLE_HPP
#define GLITCHCRAFT_CONSOLE_HPP

#include "engine.hpp"

#include <string>

namespace glitchcraft {

enum class Key {
    None,
    Up,
    Down,
    Left,
    Right,
    Forward,
    Backward,
    StrafeLeft,
    StrafeRight,
    Jump,
    ToggleFlight,
    Mine,
    Place,
    Inventory,
    CloseInventory,
    Potion,
    Milk,
    Pause,
    Confirm,
    Back,
    Realms,
    Quit,
    EatCake,
    EatCookie,
    EatCrackers,
    EatSandwich,
    EatSpaghetti,
    EatPizza,
    EatCottonCandy,
    SelectGrass,
    SelectDirt,
    SelectStone,
    SelectWood,
    SelectRedstone,
    SelectWater,
    SelectLava,
    CycleBuildMode,
    SelectNextBuildItem,
    SelectPreviousBuildItem,
    RotateTrack,
    ToggleTrainPower,
    RefuelTrain,
    ToggleTrackSwitch,
};

class Console {
public:
    Console();
    ~Console();
    Console(const Console &) = delete;
    Console &operator=(const Console &) = delete;

    int width() const noexcept;
    int height() const noexcept;
    Key readKey(bool inventoryMode = false);
    std::string readText(const std::string &prompt);
    void drawTitle();
    void drawTerms();
    void drawWorldSelect();
    void drawRealms();
    void drawInventory(GcBlock selected, int foodLevel);
    void drawPaused();
    void drawGame(const Game &game);
    void drawMessage(const std::string &message);
    void clear();
    static bool interactive() noexcept;

private:
    int width_;
    int height_;
    bool terminalMode_;
    bool rawMode_;
    int originalFlags_;
    void drawTextScreen(const std::string &title,
                        const std::string &body,
                        const std::string &footer);
};

}

#endif
