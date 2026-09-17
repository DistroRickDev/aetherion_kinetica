#ifndef AETHERION_KINETICA_ENGINE_HANDLER_H
#define AETHERION_KINETICA_ENGINE_HANDLER_H

#include <list>
#include <memory>

#include "core/gui/EngineGui.h"
#include "graphics/GpuDeviceHandler.h"
#include "window/WindowHandler.h"

namespace engine::core::handler {
    class EngineHandler {
    public:
        enum class ModuleType : std::uint32_t {
            AUDIO = 0x00000010u,
            VIDEO = 0x00000020u,
            JOYSTICK = 0x00000200u,
            HAPTIC = 0x00001000u,
            GAMEPAD = 0x00002000u,
            EVENTS = 0x00004000u,
            SENSOR = 0x00008000u,
            CAMERA = 0x00010000u
        };

        EngineHandler() = default;

        EngineHandler(EngineHandler const &) = delete;

        void operator=(EngineHandler const &) = delete;

        ~EngineHandler();

        void initialize(const std::list<ModuleType> &moduleTypes = {ModuleType::VIDEO}) noexcept;

        void run(int argc , const char **argv) const;

        const std::unique_ptr<graphics::GpuDeviceHandler>& getGpuDeviceHandler() noexcept;

        const std::unique_ptr<gui::EngineGui>& getGuiHandler() noexcept;

        const std::unique_ptr<window::WindowHandler>& getWindowHandler() noexcept;

        void createSecondaryWindow() noexcept;

    private:
        void releaseAllResources() const;

        void terminateAllResources();

        std::unique_ptr<graphics::GpuDeviceHandler> gpuDeviceHandler;
        std::unique_ptr<gui::EngineGui> engineGuiHandler;
        std::unique_ptr<window::WindowHandler> windowHandler;
    };
}

#endif //AETHERION_KINETICA_ENGINE_HANDLER_H
