#include "core/render/EngineRenderer.h"

#include <cstring>

#include "core/logger/SdlLoggerHandler.h"

#include <stdexcept>

#include "SDL3/SDL_gpu.h"

namespace {
    using namespace engine::core;
    using Logger = logger::SdlLoggerHandler;
    SDL_GPUColorTargetDescription exampleColorTargetDescription[1];
    SDL_GPUBufferCreateInfo innerBufferCreateInfo;
    SDL_GPUTransferBufferCreateInfo innerBufferTransferCreateInfo;

    SDL_GPUBufferUsageFlags fromGpuBufferUserFlags(const render::GpuBufferUserFlags userFlag) {
        switch (userFlag) {
            case render::GpuBufferUserFlags::SDL_GPU_BUFFER_USAGE_VERTEX:
                return SDL_GPU_BUFFERUSAGE_VERTEX;
            case render::GpuBufferUserFlags::SDL_GPU_BUFFER_USAGE_INDEX:
                return SDL_GPU_BUFFERUSAGE_INDEX;
            case render::GpuBufferUserFlags::SDL_GPU_BUFFER_USAGE_INDIRECT:
                return SDL_GPU_BUFFERUSAGE_INDIRECT;
            case render::GpuBufferUserFlags::SDL_GPU_BUFFER_USAGE_GRAPHICS_STORAGE_READ:
                return SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
            case render::GpuBufferUserFlags::SDL_GPU_BUFFER_USAGE_COMPUTE_STORAGE_READ:
                return SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ;
            case render::GpuBufferUserFlags::SDL_GPU_BUFFER_USAGE_COMPUTE_STORAGE_WRITE:
                return SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
        }
        return {};
    }

    SDL_GPUBufferCreateInfo fromGpuBufferInfo(const render::GpuBufferInfo &gpuBufferInfo) {
        return {
            .usage = fromGpuBufferUserFlags(gpuBufferInfo.usage),
            .size = gpuBufferInfo.size,
            .props = gpuBufferInfo.props,
        };
    }
}

GpuGraphicsPipeline *render::EngineRenderer::createGpuGraphicsPipeline(GpuShader *vertexShader,
                                                                       GpuShader *fragmentShader, GpuDevice *device,
                                                                       Window *window) {
    exampleColorTargetDescription[0] = {
        .format = SDL_GetGPUSwapchainTextureFormat(device, window)
    };
    const SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .vertex_shader = vertexShader,
        .fragment_shader = fragmentShader,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = {
            .fill_mode = SDL_GPU_FILLMODE_FILL
        },
        .target_info = {
            .color_target_descriptions = exampleColorTargetDescription,
            .num_color_targets = 1,
        },
    };
    const auto result{SDL_CreateGPUGraphicsPipeline(device, &pipelineCreateInfo)};
    if (!result) {
        throw std::runtime_error("Failed to create GPU GraphicsPipeline");
    }
    return result;
}

void render::EngineRenderer::releaseGpuGraphicsPipeline(GpuDevice *device, GpuGraphicsPipeline *pipeline) {
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
}

GpuCommandBuffer *render::EngineRenderer::acquireGpuCommandBuffer(GpuDevice *device) {
    const auto commandBuffer{SDL_AcquireGPUCommandBuffer(device)};
    if (!commandBuffer) {
        throw std::runtime_error("Failed to acquire GPU command buffer");
    }
    return commandBuffer;
}

GpuTexture *render::EngineRenderer::acquireGpuTexture(GpuCommandBuffer *commandBuffer, Window *window) {
    SDL_GPUTexture *swapChainTexture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapChainTexture, nullptr, nullptr)) {
        throw std::runtime_error("failed to acquire swap chain texture");
    }
    return swapChainTexture;
}

void render::EngineRenderer::drawPrimitives(GpuBuffer *buffer, GpuTexture *texture, GpuCommandBuffer *commandBuffer,
                                            GpuGraphicsPipeline *pipeline, const PrimitiveInfo &primitiveInfo) {
    const auto colorTargetInfo{
        SDL_GPUColorTargetInfo{
            .texture = texture,
            .clear_color = (SDL_FColor){.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f},
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        }
    };
    Logger::debug("SDL_GPUColorTargetInfo created successfully");
    SDL_GPURenderPass *renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, nullptr);
    Logger::debug("SDL_GPURenderPass created successfully");
    SDL_BindGPUGraphicsPipeline(renderPass, pipeline);
    Logger::debug("GraphicsPipeline bound successfully");

    const auto gpuBufferBinding{SDL_GPUBufferBinding{.buffer = buffer, .offset = 0}};
    SDL_BindGPUVertexBuffers(renderPass, 0, &gpuBufferBinding, 1);
    Logger::debug("GPUVertexBuffers bound successfully");

    const auto textureSamplerBinding{SDL_GPUTextureSamplerBinding{}};
    SDL_BindGPUFragmentSamplers(renderPass, 1, &textureSamplerBinding,1);
    Logger::debug("GPUFragmentSamplers bound successfully");

    SDL_DrawGPUPrimitives(renderPass, primitiveInfo.num_vertices, primitiveInfo.num_instances,
                          primitiveInfo.first_vertex, primitiveInfo.first_instance);
    Logger::debug("SDL_DrawGPUPrimitives called successfully");
    SDL_EndGPURenderPass(renderPass);
    Logger::debug("SDL_EndGPURenderPass called successfully");
}

void render::EngineRenderer::submitGpuCommandBuffer(GpuCommandBuffer *commandBuffer) {
    SDL_SubmitGPUCommandBuffer(commandBuffer);
}

GpuBuffer *render::EngineRenderer::createGpuBuffer(GpuDevice *device, const GpuBufferInfo &gpuBufferInfo) {
    innerBufferCreateInfo = fromGpuBufferInfo(gpuBufferInfo);
    const auto buffer{
        SDL_CreateGPUBuffer(device, &innerBufferCreateInfo)
    };
    if (!buffer) {
        throw std::runtime_error("Failed to create GPU buffer");
    }
    return buffer;
}

void render::EngineRenderer::uploadVertices(GpuDevice *device, SDL_GPUBuffer *gpuBuffer,
                                            const std::vector<Vertex> &vertices) {
    /* ── Upload vertices ── */
    const auto sizeOfVertices{vertices.size() * sizeof(Vertex)};
    innerBufferTransferCreateInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = static_cast<std::uint32_t>(sizeOfVertices)
    };

    SDL_GPUTransferBuffer *stg = SDL_CreateGPUTransferBuffer(device, &innerBufferTransferCreateInfo);
    void *ptr = SDL_MapGPUTransferBuffer(device, stg, false);
    std::memcpy(ptr, vertices.data(), sizeOfVertices);
    SDL_UnmapGPUTransferBuffer(device, stg); {
        auto *commandBuffer{SDL_AcquireGPUCommandBuffer(device)};
        auto *copyPass{SDL_BeginGPUCopyPass(commandBuffer)};
        const auto bufferLocation{SDL_GPUTransferBufferLocation{stg, 0}};
        const auto bufferRegion{
            SDL_GPUBufferRegion{.buffer = gpuBuffer, .offset = 0, .size = static_cast<std::uint32_t>(sizeOfVertices)}
        };

        SDL_UploadToGPUBuffer(copyPass, &bufferLocation, &bufferRegion, false);
        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        SDL_WaitForGPUIdle(device);
    }
    SDL_ReleaseGPUTransferBuffer(device, stg);
}

void render::EngineRenderer::releaseGpuBuffer(GpuDevice *device, GpuBuffer *gpuBuffer) {
    SDL_ReleaseGPUBuffer(device, gpuBuffer);
}
