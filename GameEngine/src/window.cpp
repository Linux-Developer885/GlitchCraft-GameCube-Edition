#include "GameEngine/window.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#ifdef GLITCHCRAFT_HAS_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <cstdlib>
#endif

namespace glitchcraft::gameengine {

struct Window::Platform {
#ifdef GLITCHCRAFT_HAS_X11
    Display *display = nullptr;
    ::Window window = 0;
    GC graphics = 0;
    Atom deleteMessage = 0;
    XImage *image = nullptr;
    int width = 0;
    int height = 0;
    bool open = false;
#endif
};

Window::Window() : platform_(std::make_unique<Platform>()) {}

Window::~Window()
{
    close();
}

bool Window::open(int width, int height)
{
#ifdef GLITCHCRAFT_HAS_X11
    if (platform_->open) {
        return true;
    }
    platform_->display = XOpenDisplay(nullptr);
    if (platform_->display == nullptr) {
        return false;
    }
    const int screen = DefaultScreen(platform_->display);
    platform_->width = width;
    platform_->height = height;
    platform_->window = XCreateSimpleWindow(
        platform_->display, RootWindow(platform_->display, screen),
        30, 30, static_cast<unsigned int>(width),
        static_cast<unsigned int>(height), 0,
        BlackPixel(platform_->display, screen),
        BlackPixel(platform_->display, screen));
    XStoreName(platform_->display, platform_->window,
               "GlitchCraft: GameCube Edition");
    XSelectInput(platform_->display, platform_->window,
                 ExposureMask | KeyPressMask | StructureNotifyMask);
    platform_->deleteMessage =
        XInternAtom(platform_->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(platform_->display, platform_->window,
                    &platform_->deleteMessage, 1);
    platform_->graphics = XCreateGC(platform_->display,
                                    platform_->window, 0, nullptr);
    platform_->image = XCreateImage(
        platform_->display, DefaultVisual(platform_->display, screen),
        static_cast<unsigned int>(DefaultDepth(platform_->display, screen)),
        ZPixmap, 0, nullptr, static_cast<unsigned int>(width),
        static_cast<unsigned int>(height), 32, 0);
    if (platform_->image == nullptr) {
        close();
        return false;
    }
    const std::size_t imageBytes =
        static_cast<std::size_t>(platform_->image->bytes_per_line) *
        static_cast<std::size_t>(height);
    platform_->image->data = static_cast<char *>(std::calloc(imageBytes, 1U));
    if (platform_->image->data == nullptr) {
        close();
        return false;
    }
    platform_->open = true;
    XMapWindow(platform_->display, platform_->window);
    XFlush(platform_->display);
    return true;
#else
    (void)width;
    (void)height;
    return false;
#endif
}

void Window::close()
{
#ifdef GLITCHCRAFT_HAS_X11
    if (!platform_ || platform_->display == nullptr) {
        return;
    }
    platform_->open = false;
    if (platform_->image != nullptr) {
        XDestroyImage(platform_->image);
        platform_->image = nullptr;
    }
    if (platform_->graphics != 0) {
        XFreeGC(platform_->display, platform_->graphics);
        platform_->graphics = 0;
    }
    if (platform_->window != 0) {
        XDestroyWindow(platform_->display, platform_->window);
        platform_->window = 0;
    }
    XCloseDisplay(platform_->display);
    platform_->display = nullptr;
#endif
}

bool Window::isOpen() const noexcept
{
#ifdef GLITCHCRAFT_HAS_X11
    return platform_ && platform_->open;
#else
    return false;
#endif
}

bool Window::pollKey(int &key)
{
#ifdef GLITCHCRAFT_HAS_X11
    if (!isOpen() || XPending(platform_->display) == 0) {
        return false;
    }
    XEvent event{};
    XNextEvent(platform_->display, &event);
    if (event.type == ClientMessage &&
        static_cast<Atom>(event.xclient.data.l[0]) ==
            platform_->deleteMessage) {
        key = 'q';
        return true;
    }
    if (event.type != KeyPress) {
        return false;
    }
    KeySym symbol = NoSymbol;
    char text[8]{};
    XLookupString(&event.xkey, text, sizeof(text), &symbol, nullptr);
    if (symbol == XK_Escape) {
        key = 27;
    } else if (symbol == XK_Left) {
        key = 0x101;
    } else if (symbol == XK_Right) {
        key = 0x102;
    } else if (symbol == XK_Up) {
        key = 0x103;
    } else if (symbol == XK_Down) {
        key = 0x104;
    } else if (text[0] != '\0') {
        key = static_cast<unsigned char>(text[0]);
    } else {
        return false;
    }
    return true;
#else
    (void)key;
    return false;
#endif
}

void Window::present(const Renderer &renderer, const Game &game)
{
#ifdef GLITCHCRAFT_HAS_X11
    if (!isOpen()) {
        return;
    }
    const auto &pixels = renderer.pixels();
    const int sourceWidth = renderer.width();
    const int sourceHeight = renderer.height();
    const auto maskParameters = [](unsigned long mask) {
        unsigned int shift = 0;
        while (mask != 0UL && ((mask >> shift) & 1UL) == 0UL) {
            ++shift;
        }
        return std::pair<unsigned int, unsigned long>{
            shift, mask >> shift};
    };
    const auto redParameters = maskParameters(platform_->image->red_mask);
    const auto greenParameters = maskParameters(platform_->image->green_mask);
    const auto blueParameters = maskParameters(platform_->image->blue_mask);
    const auto scaleMask = [](int channel,
                              const std::pair<unsigned int, unsigned long> &parameters,
                              unsigned long mask) {
        if (mask == 0UL) {
            return 0UL;
        }
        return ((static_cast<unsigned long>(channel) * parameters.second / 255UL)
                << parameters.first) &
               mask;
    };

    for (int y = 0; y < platform_->height; ++y) {
        const int sourceY = y * sourceHeight / platform_->height;
        for (int x = 0; x < platform_->width; ++x) {
            const int sourceX = x * sourceWidth / platform_->width;
            const std::uint32_t color =
                pixels[static_cast<std::size_t>(sourceY) *
                           static_cast<std::size_t>(sourceWidth) +
                       static_cast<std::size_t>(sourceX)];
            const unsigned long red = scaleMask(
                static_cast<int>((color >> 16U) & 0xffU),
                redParameters, platform_->image->red_mask);
            const unsigned long green = scaleMask(
                static_cast<int>((color >> 8U) & 0xffU),
                greenParameters, platform_->image->green_mask);
            const unsigned long blue = scaleMask(
                static_cast<int>(color & 0xffU),
                blueParameters, platform_->image->blue_mask);
            const unsigned long nativePixel = red | green | blue;
            char *destination = platform_->image->data +
                static_cast<std::ptrdiff_t>(y) *
                    platform_->image->bytes_per_line +
                static_cast<std::ptrdiff_t>(x) *
                    (platform_->image->bits_per_pixel / 8);
            for (int byte = 0; byte < platform_->image->bits_per_pixel / 8;
                 ++byte) {
                const int shift = platform_->image->byte_order == LSBFirst
                    ? byte * 8
                    : (platform_->image->bits_per_pixel / 8 - 1 - byte) * 8;
                destination[byte] =
                    static_cast<char>((nativePixel >> shift) & 0xffUL);
            }
        }
    }
    XPutImage(platform_->display, platform_->window, platform_->graphics,
              platform_->image, 0, 0, 0, 0,
              static_cast<unsigned int>(platform_->width),
              static_cast<unsigned int>(platform_->height));
    XSetForeground(platform_->display, platform_->graphics,
                   WhitePixel(platform_->display,
                              DefaultScreen(platform_->display)));
    const std::string status =
        "WASD move  Arrows look  Space jump  F fly  R mine  P place  X inventory"
        "  Esc pause  Q quit";
    XDrawString(platform_->display, platform_->window, platform_->graphics,
                18, platform_->height - 22, status.c_str(),
                static_cast<int>(status.size()));
    const std::string stats =
        "Food " + std::to_string(game.foodLevel()) + "/100" +
        (game.flying() ? "  Flying" : "") +
        (game.potionTicks() > 0
             ? "  Potion " + std::to_string(game.potionTicks() / 60) + "s"
             : "");
    XDrawString(platform_->display, platform_->window, platform_->graphics,
                18, 24, stats.c_str(), static_cast<int>(stats.size()));
    const int centerX = platform_->width / 2;
    const int centerY = platform_->height / 2;
    XDrawLine(platform_->display, platform_->window, platform_->graphics,
              centerX - 8, centerY, centerX + 8, centerY);
    XDrawLine(platform_->display, platform_->window, platform_->graphics,
              centerX, centerY - 8, centerX, centerY + 8);
    XFlush(platform_->display);
#else
    (void)renderer;
    (void)game;
#endif
}

void Window::drawOverlay(const std::string &title,
                         const std::vector<std::string> &lines)
{
#ifdef GLITCHCRAFT_HAS_X11
    if (!isOpen()) {
        return;
    }
    const int boxWidth = std::min(platform_->width - 48, 680);
    const int boxHeight = std::min(
        platform_->height - 48, 96 + static_cast<int>(lines.size()) * 30);
    const int left = (platform_->width - boxWidth) / 2;
    const int top = (platform_->height - boxHeight) / 2;
    XSetForeground(platform_->display, platform_->graphics,
                   BlackPixel(platform_->display,
                              DefaultScreen(platform_->display)));
    XFillRectangle(platform_->display, platform_->window, platform_->graphics,
                   left, top, static_cast<unsigned int>(boxWidth),
                   static_cast<unsigned int>(boxHeight));
    XSetForeground(platform_->display, platform_->graphics,
                   WhitePixel(platform_->display,
                              DefaultScreen(platform_->display)));
    XDrawString(platform_->display, platform_->window, platform_->graphics,
                left + 24, top + 38, title.c_str(),
                static_cast<int>(title.size()));
    int y = top + 70;
    for (const auto &line : lines) {
        XDrawString(platform_->display, platform_->window, platform_->graphics,
                    left + 24, y, line.c_str(), static_cast<int>(line.size()));
        y += 26;
    }
    XFlush(platform_->display);
#else
    (void)title;
    (void)lines;
#endif
}

}
