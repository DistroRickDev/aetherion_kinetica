#include "WindowHandler.h"

#include <algorithm>

#include "core/logger/SdlLoggerHandler.h"

#include <SDL3/SDL_video.h>

#include <format>
#include <ranges>
#include <utility>

namespace {
    using Logger = engine::core::logger::SdlLoggerHandler;

    SDL_Window *
    createWindowFromSettings(const engine::window::WindowHandler::WindowSettings &window_settings) noexcept {
        auto flags{0U};
        for (auto flag: window_settings.mFlags) {
            flags |= static_cast<uint32_t>(flag);
        }
        return SDL_CreateWindow(window_settings.mTitle.c_str(), window_settings.mWidth, window_settings.mHeight, flags);
    }
}

namespace engine::window {
    WindowHandler::WindowHandler(const WindowSettings &windowSettings)
        : _settingsWindow(std::make_shared<WindowSettings>(windowSettings)) {
        this->_windows[WindowType::MainWindow] = createWindowFromSettings(*this->_settingsWindow);
        core::logger::SdlLoggerHandler::debug(
            std::format("Created window with id {}", SDL_GetWindowID(this->_windows[WindowType::MainWindow])));
    }

    WindowHandler::WindowSettings::WindowSettings(std::string title, const std::int32_t width,
                                                  const std::int32_t height,
                                                  std::list<WindowFlags> flags) : mTitle(std::move(title)),
        mWidth(width), mHeight(height), mFlags(std::move(flags)) {
    }

    WindowHandler::~WindowHandler() {
        core::logger::ILogger::debug("Destroying main window");
        if (!_windows.empty()) {
            for (const auto window: this->_windows | std::views::values) {
                SDL_DestroyWindow(window);
            }
        }
        _windows.clear();
    }

    void WindowHandler::createWindow(const WindowSettings &window_settings, const WindowType window_type) noexcept {
        if (auto *window = createWindowFromSettings(window_settings)) {
            const auto id{SDL_GetWindowID(window)};
            core::logger::SdlLoggerHandler::debug(std::format("Created window with id {}", id));
            this->_windows[window_type] = window;
            return;
        }
        core::logger::SdlLoggerHandler::error(std::format("Failed to create window with id {}", SDL_GetError()));
    }

    void WindowHandler::destroyWindow(const WindowType window_type) noexcept {
        if (this->_windows.empty()) {
            Logger::error("There is no windows to destroy");
            return;
        }

        const auto windowPos{_windows.find(window_type)};
        if (windowPos == _windows.end() || windowPos->second == nullptr) {
            Logger::error("Window currently not stored for destruction");
            return;
        }

        SDL_DestroyWindow(windowPos->second);
        this->_windows.erase(_windows.find(window_type));
    }

    void WindowHandler::destroyWindowById(const std::uint32_t windowId) noexcept {
        WindowType windowToDestroy{};
        std::ranges::for_each(this->_windows, [windowId, &windowToDestroy](const auto &window) {
            if (windowId == SDL_GetWindowID(window.second)) {
                windowToDestroy = window.first;
            }
        });
        this->destroyWindow(windowToDestroy);
    }

    SDL_Window *WindowHandler::getMainWindow() const {
        if (this->_windows.empty() || _windows.at(WindowType::MainWindow) == nullptr) {
            throw std::runtime_error{"Failed to get main window"};
        }
        return this->_windows.at(WindowType::MainWindow);
    }
} // namespace engine::window
