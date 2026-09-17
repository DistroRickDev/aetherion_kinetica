#ifndef AETHERION_KINETICA_WINDOW_HANDLER_H
#define AETHERION_KINETICA_WINDOW_HANDLER_H

#include <cstdint>
#include <list>
#include <memory>
#include <string>

class SDL_Window;
using Window = SDL_Window;

namespace engine::window {
    class WindowHandler {
    public:
        enum class WindowFlags : std::uint32_t {
            WINDOW_FULLSCREEN = 0x0000000000000001, /**< window is in fullscreen mode */
            WINDOW_OPENGL = 0x0000000000000002, /**< window usable with OpenGL context */
            WINDOW_OCCLUDED = 0x0000000000000004, /**< window is occluded */
            WINDOW_HIDDEN = 0x0000000000000008,
            /**< window is neither mapped onto the desktop nor shown in the taskbar/dock/window list; SDL_ShowWindow() is required for it to become visible */
            WINDOW_BORDERLESS = 0x0000000000000010, /**< no window decoration */
            WINDOW_RESIZABLE = 0x0000000000000020, /**< window can be resized */
            WINDOW_MINIMIZED = 0x0000000000000040, /**< window is minimized */
            WINDOW_MAXIMIZED = 0x0000000000000080, /**< window is maximized */
            WINDOW_MOUSE_GRABBED = 0x0000000000000100, /**< window has grabbed mouse input */
            WINDOW_INPUT_FOCUS = 0x0000000000000200, /**< window has input focus */
            WINDOW_MOUSE_FOCUS = 0x0000000000000400, /**< window has mouse focus */
            WINDOW_EXTERNAL = 0x0000000000000800, /**< window not created by SDL */
            WINDOW_MODAL = 0x0000000000001000, /**< window is modal */
            WINDOW_HIGH_PIXEL_DENSITY = 0x0000000000002000,
            /**< window uses high pixel density back buffer if possible */
            WINDOW_MOUSE_CAPTURE = 0x0000000000004000, /**< window has mouse captured (unrelated to MOUSE_GRABBED) */
            WINDOW_MOUSE_RELATIVE_MODE = 0x0000000000008000, /**< window has relative mode enabled */
            WINDOW_ALWAYS_ON_TOP = 0x0000000000010000, /**< window should always be above others */
            WINDOW_UTILITY = 0x0000000000020000,
            /**< window should be treated as a utility window, not showing in the task bar and window list */
            WINDOW_TOOLTIP = 0x0000000000040000,
            /**< window should be treated as a tooltip and does not get mouse or keyboard focus, requires a parent window */
            WINDOW_POPUP_MENU = 0x0000000000080000,
            /**< window should be treated as a popup menu, requires a parent window */
            WINDOW_KEYBOARD_GRABBED = 0x0000000000100000, /**< window has grabbed keyboard input */
            WINDOW_FILL_DOCUMENT = 0x0000000000200000,
            /**< window is in fill-document mode (Emscripten only), since SDL 3.4.0 */
            WINDOW_VULKAN = 0x0000000010000000, /**< window usable for Vulkan surface */
            WINDOW_METAL = 0x0000000020000000, /**< window usable for Metal view */
            WINDOW_TRANSPARENT = 0x0000000040000000, /**< window with transparent buffer */
            WINDOW_NOT_FOCUSABLE = 0x0000000080000000 /**< window should not be focusable */
        };

        struct WindowSettings {
            explicit WindowSettings(std::string title, std::int32_t width, std::int32_t height,
                                    std::list<WindowFlags> flags = {});

            const std::string mTitle;
            const std::int32_t mWidth;
            const std::int32_t mHeight;
            const std::list<WindowFlags> mFlags;
        };

        explicit WindowHandler(const WindowSettings &windowSettings);

        WindowHandler(WindowHandler const &) = delete;

        void operator=(WindowHandler const &) = delete;

        ~WindowHandler();

        std::uint32_t createWindow(const WindowSettings &window_settings) noexcept;

        void destroyWindow(std::uint32_t windowId) noexcept;

        [[nodiscard]] Window *getMainWindow() const;

    private:
        std::shared_ptr<WindowSettings> _settingsWindow;
        Window *_main_window;
        std::list<Window *> _windows{};
    };
} // namespace engine::window
#endif
