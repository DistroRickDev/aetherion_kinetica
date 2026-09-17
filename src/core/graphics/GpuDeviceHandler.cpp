#include <format>

#include "GpuDeviceHandler.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

namespace {
}

namespace engine::graphics {
    GpuDeviceHandler::GpuDeviceHandler(const bool enableDebugMode) {
        this->_device = SDL_CreateGPUDevice(
            SDL_GPU_SHADERFORMAT_SPIRV, /* Only support Vulkan for now */
            enableDebugMode, /* debug_mode */
            nullptr /* auto-select backend */
        );
    }

    GpuDeviceHandler::~GpuDeviceHandler() {
        SDL_DestroyGPUDevice(this->_device);
    }

    SDL_GPUDevice *GpuDeviceHandler::getDevice() const {
        return this->_device;
    }

    std::string GpuDeviceHandler::getDeviceDriver() const {
        return {SDL_GetGPUDeviceDriver(this->_device)};
    }

    bool GpuDeviceHandler::waitForGpu() const {
        return SDL_WaitForGPUIdle(this->_device);
    }

    bool GpuDeviceHandler::claimWindow(SDL_Window *window) const {
        return SDL_ClaimWindowForGPUDevice(this->_device, window);
    }

    void GpuDeviceHandler::releaseWindowFromGpuDevice(Window *window) const {
        SDL_ReleaseWindowFromGPUDevice(this->_device, window);
    }
}
