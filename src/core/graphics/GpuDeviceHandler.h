#ifndef AETHERION_KINETICA_GPU_DEVICE_HANDLER_H
#define AETHERION_KINETICA_GPU_DEVICE_HANDLER_H

#include <string>

class SDL_GPUCommandBuffer;
class SDL_GPUDevice;
class SDL_GPUTexture;
class SDL_Window;

using Device = SDL_GPUDevice;
using GpuCommandBuffer = SDL_GPUCommandBuffer;
using GpuTexture = SDL_GPUTexture;
using Window = SDL_Window;

namespace engine::graphics {
    struct GpuDeviceHandler {
        explicit GpuDeviceHandler(bool enableDebugMode);

        ~GpuDeviceHandler();

        [[nodiscard]] Device *getDevice() const;

        [[nodiscard]] std::string getDeviceDriver() const;

        [[nodiscard]] bool waitForGpu() const;

        bool claimWindow(Window *window) const;

        void releaseWindowFromGpuDevice(Window *window) const;

        [[nodiscard]] GpuCommandBuffer* acquireCommandBuffer() const;



    private:
        Device *_device;
    };
}
#endif
