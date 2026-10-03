//
// Created by ditrorickdev on 26/08/2026.
//

#ifndef AETHERION_KINETICA_ENGINE_RENDERER_H
#define AETHERION_KINETICA_ENGINE_RENDERER_H

#include <cstdint>
#include <vector>

class SDL_GPUBuffer;
class SDL_GPUCommandBuffer;
class SDL_GPUDevice;
class SDL_GPUGraphicsPipeline;
class SDL_GPUShader;
class SDL_GPUTexture;
class SDL_Window;

using GpuBuffer = SDL_GPUBuffer;
using GpuCommandBuffer = SDL_GPUCommandBuffer;
using GpuDevice = SDL_GPUDevice;
using GpuGraphicsPipeline = SDL_GPUGraphicsPipeline;
using GpuShader = SDL_GPUShader;
using GpuTexture = SDL_GPUTexture;
using Window = SDL_Window;

namespace engine::core::render {
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

    struct PrimitiveInfo {
        std::uint32_t num_vertices;     // the number of vertices
        std::uint32_t num_instances;    // the number of instances
        std::uint32_t first_vertex;     // the first vertex
        std::uint32_t first_instance;   // the first instance
    } __attribute__((packed));


    enum class GpuBufferUserFlags : std::uint32_t {
        SDL_GPU_BUFFER_USAGE_VERTEX,                 // Buffer is a vertex buffer.
        SDL_GPU_BUFFER_USAGE_INDEX,                  // Buffer is an index buffer.
        SDL_GPU_BUFFER_USAGE_INDIRECT,               // Buffer is an indirect buffer.
        SDL_GPU_BUFFER_USAGE_GRAPHICS_STORAGE_READ,  // Buffer supports storage reads in graphics stages.
        SDL_GPU_BUFFER_USAGE_COMPUTE_STORAGE_READ,   // Buffer supports storage reads in the compute stage.
        SDL_GPU_BUFFER_USAGE_COMPUTE_STORAGE_WRITE,  // Buffer supports storage writes in the compute stage.
    };

    struct GpuBufferInfo {
        GpuBufferUserFlags usage;   // How the buffer is intended to be used
        std::uint32_t size;         // The size in bytes of the buffer.
        std::uint32_t props;        // A properties ID for extensions. Should be 0 if none
    }__attribute__((packed));

    class EngineRenderer {
    public:
        EngineRenderer() = default;

        EngineRenderer(EngineRenderer const &) = delete;

        void operator=(EngineRenderer const &) = delete;

        ~EngineRenderer() = default;

        static GpuGraphicsPipeline *createGpuGraphicsPipeline(GpuShader *vertexShader, GpuShader *fragmentShader,
                                                              GpuDevice *device, Window *window);

        static void releaseGpuGraphicsPipeline(GpuDevice *device, GpuGraphicsPipeline *pipeline);

        static GpuCommandBuffer* acquireGpuCommandBuffer(GpuDevice *device);

        static GpuTexture *acquireGpuTexture(GpuCommandBuffer *commandBuffer, Window *window);

        static void drawPrimitives(GpuBuffer* buffer, GpuTexture *texture, GpuCommandBuffer *commandBuffer, GpuGraphicsPipeline *pipeline, const PrimitiveInfo& primitiveInfo);

        static void submitGpuCommandBuffer(GpuCommandBuffer *commandBuffer);

        static GpuBuffer* createGpuBuffer(GpuDevice* device, const GpuBufferInfo& gpuBufferInfo);

        static void uploadVertices(GpuDevice* device, SDL_GPUBuffer* gpuBuffer, const std::vector<Vertex>& vertices);

        static void releaseGpuBuffer(GpuDevice *device, GpuBuffer* gpuBuffer);
    };
}
#endif // AETHERION_KINETICA_ENGINE_RENDERER_H
