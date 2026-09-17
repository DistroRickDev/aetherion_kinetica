//
// Created by ditrorickdev on 26/08/2026.
//

#ifndef AETHERION_KINETICA_ENGINE_RENDERER_H
#define AETHERION_KINETICA_ENGINE_RENDERER_H

#include <cstring>
#include <format>
#include <iostream>
#include <vector>
#include <SDL3/SDL.h>

namespace core::render {
    struct Vertex {
        alignas(4) float x; // Position X
        alignas(4) float y; // Position Y
        alignas(4) float z; // Position Z

        alignas(4) float r; // Color R
        alignas(4) float g; // Color G
        alignas(4) float b; // Color B
        alignas(4) float a; // Color A

        alignas(4) float u; // Texture UV U
        alignas(4) float v; // Texture UV V
    };

    /**
 * Creates a triangle with vertices at (0, 0.5), (-0.5, -0.5), and (0.5, -0.5).
 * Colors: Top (Red), Bottom-Left (Green), Bottom-Right (Blue).
 */
    constexpr std::vector<Vertex> createExampleTriangleVertices() {
        return {
            // Top vertex
            {
                .x = 0.0f, .y = 0.5f, .z = 0.0f,
                .r = 1.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f,
                .u = 0.5f, .v = 0.0f
            },
            // Bottom-left vertex
            {
                .x = -0.5f, .y = -0.5f, .z = 0.0f,
                .r = 0.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f,
                .u = 0.0f, .v = 1.0f
            },
            // Bottom-right vertex
            {
                .x = 0.5f, .y = -0.5f, .z = 0.0f,
                .r = 0.0f, .g = 0.0f, .b = 1.0f, .a = 1.0f,
                .u = 1.0f, .v = 1.0f
            }
        };
    }
}

namespace VertexData {


    inline std::uint32_t getVerticesCount(const std::vector<Vertex> &vertices) {
        return static_cast<std::uint32_t>(vertices.size());
    }

    inline std::uint32_t getVerticesBufferSize(const std::vector<Vertex> &vertices) {
        return static_cast<std::uint32_t>(vertices.size() * sizeof(Vertex));
    }
} // namespace TriangleData

// --- GPU Helper Functions ---

inline bool checkTextureProperties(SDL_GPUDevice *device) {
    if (!device) {
        throw std::runtime_error("Device is invalid");
    }

    return SDL_GPUTextureSupportsFormat(
        device,
        SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        SDL_GPU_TEXTURETYPE_2D,
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET /* or SAMPLER, STORAGE, etc. */
    );
}

namespace TriangleRenderer {
    /**
     * Creates the GPU buffer info for vertex data.
     * @return SDL_GPUBufferCreateInfo object
     */
    inline SDL_GPUBufferCreateInfo createVertexBufferCreateInfo(
        const std::vector<Vertex> &vertices
    ) {
        return SDL_GPUBufferCreateInfo{
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = VertexData::getVerticesBufferSize(vertices),
            .props = 0
        };
    }

    /**
     * Creates a transfer buffer info for uploading data from CPU to GPU.
     */
    inline SDL_GPUTransferBufferCreateInfo createUploadTransferBufferCreateInfo(
        const std::vector<Vertex> &vertices
    ) {
        return SDL_GPUTransferBufferCreateInfo{
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = VertexData::getVerticesBufferSize(vertices)
        };
    }

    /**
     * Creates a GPU Vertex Buffer.
     * @param device The GPU device
     * @param info The creation info
     * @param name Optional name for debugging
     * @return SDL_GPUBuffer*
     */
    inline SDL_GPUBuffer *createVertexBuffer(
        SDL_GPUDevice *device,
        const SDL_GPUBufferCreateInfo &info,
        const std::string &name = ""
    ) {
        SDL_GPUBuffer *buffer = SDL_CreateGPUBuffer(device, &info);

        if (!buffer) {
            throw std::runtime_error(
                std::format("Failure to create GPU buffer with error: '{}'", SDL_GetError()).c_str());
        }

        if (!name.empty()) {
            SDL_SetGPUBufferName(device, buffer, name.c_str());
        }

        return buffer;
    }

    /**
     * Creates a GPU Transfer Buffer.
     */
    inline SDL_GPUTransferBuffer *createUploadTransferBuffer(
        SDL_GPUDevice *device,
        const SDL_GPUTransferBufferCreateInfo &info
    ) {
        return SDL_CreateGPUTransferBuffer(device, &info);
    }

    /**
     * Maps the transfer buffer and uploads vertex data.
     * @param device The GPU device
     * @param transfer The transfer buffer handle
     * @param vertices The vertex data to upload
     * @param cycle When cycle = true, SDL internally uses a new chunk of the transfer buffer even if you're
     * true for per-frame dynamic data that you update every frame without waiting.
     * @note This performs a synchronous upload. Ensure this is called before using the buffer in a render pass.
     */
    inline void uploadVertexData(
        SDL_GPUDevice *device,
        SDL_GPUTransferBuffer *transfer,
        const std::vector<Vertex> &vertices,
        const bool cycle = false
    ) {
        if (!device || !transfer) {
            throw std::runtime_error("Device or Transfer buffer is invalid!");
        }

        if (vertices.empty()) {
            std::cout << "Vertices are empty!" << std::endl;
            return;
        }

        // Map the transfer buffer to get a CPU pointer
        void *ptr = SDL_MapGPUTransferBuffer(device, transfer, cycle);
        if (!ptr) {
            throw std::runtime_error(
                std::format("Failed to create GPU transfer buffer with error {}!", SDL_GetError()).c_str());
        }

        // Copy data
        std::memcpy(ptr, vertices.data(), sizeof(Vertex) * vertices.size());

        // Unmap immediately after upload (synchronous)
        SDL_UnmapGPUTransferBuffer(device, transfer);
    }

    inline void recordCopyCommand(SDL_GPUDevice *device,  SDL_GPUBuffer* buffer, SDL_GPUTransferBuffer *transfer, const std::vector<Vertex> &vertices) {
        if (!device || !transfer || !buffer) {
            throw std::runtime_error("Device or Transfer buffer or SDL_GPUBuffer is invalid!");
        }

        /* 3. Record a copy command */
        auto *cmd {SDL_AcquireGPUCommandBuffer(device)};
        auto *copy {SDL_BeginGPUCopyPass(cmd)};

        const SDL_GPUTransferBufferLocation src = {
            .transfer_buffer = transfer,
            .offset          = 0
        };
        const SDL_GPUBufferRegion dst = {
            .buffer = buffer,
            .offset = 0,
            .size   = VertexData::getVerticesBufferSize(vertices),
        };
        SDL_UploadToGPUBuffer(copy, &src, &dst, false /* cycle */);
        SDL_EndGPUCopyPass(copy);
    }
} // namespace TriangleRenderer

#endif // AETHERION_KINETICA_ENGINE_RENDERER_H
