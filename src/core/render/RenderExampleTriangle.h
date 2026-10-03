#ifndef AETHERION_KINETICA_RENDER_EXAMPLE_TRIANGLE_H
#define AETHERION_KINETICA_RENDER_EXAMPLE_TRIANGLE_H

#include <format>

#include "EngineRenderer.h"
#include <core/shaders/ShaderHandler.h>

namespace engine::core::render {
    static GpuBuffer *buffer;
    static GpuGraphicsPipeline *graphicsPipeline;
    static GpuCommandBuffer *commandBuffer;
    static GpuTexture *texture;

    constexpr auto kTestShaderPath{"/home/distrorickdev/projects/aetherion_kinetica/examples/shaders/glsl"};
    const std::vector<Vertex> kTestTriangleVertices = {
        {.x = 0.0f, .y = 0.5f, .z = 0.0f, .r = 1, .g = 0, .b = 0, .a = 1},
        {.x = -0.5f, .y = -0.5f, .z = 0.0f, .r = 0, .g = 1, .b = 0, .a = 1},
        {.x = 0.5f, .y = -0.5f, .z = 0.0f, .r = 0, .g = 0, .b = 1, .a = 1},
    };

    class ExampleTriangleRender {
    public:
        ExampleTriangleRender() = default;

        ExampleTriangleRender(ExampleTriangleRender const &) = delete;

        void operator=(ExampleTriangleRender const &) = delete;

        ~ExampleTriangleRender();

        static void init(Device *device, Window *window) {
            const auto vertexSize{sizeof(Vertex) * kTestTriangleVertices.size()};
            buffer = EngineRenderer::createGpuBuffer(device, {
                                                         .usage = GpuBufferUserFlags::SDL_GPU_BUFFER_USAGE_VERTEX,
                                                         .size = static_cast<std::uint32_t>(vertexSize), .props = 0
                                                     });
            logger::ILogger::debug("createGpuBuffer successfully");
            EngineRenderer::uploadVertices(device, buffer, kTestTriangleVertices);
            logger::ILogger::debug("uploadVertices successfully");

            const auto vertexShaderFilePath{std::format("{}/{}", kTestShaderPath, "shader.vert.spv")};
            const auto fragmentShaderFilePath{std::format("{}/{}", kTestShaderPath, "shader.frag.spv")};

            logger::ILogger::debug("Creating graphics pipeline ...");
            graphicsPipeline = EngineRenderer::createGpuGraphicsPipeline(vertexShaderFilePath, fragmentShaderFilePath, device, window);
            logger::ILogger::debug("Graphics Pipeline created successfully");
        }

        static void renderTriangle(Device *device, Window *window) {
            commandBuffer = EngineRenderer::acquireGpuCommandBuffer(device);
            logger::ILogger::debug("CommandBuffer created successfully");
            texture = EngineRenderer::acquireGpuTexture(commandBuffer, window);
            logger::ILogger::debug("Graphics Texture created successfully");

            if (texture) {
                EngineRenderer::drawPrimitives(buffer, texture, commandBuffer, graphicsPipeline, PrimitiveInfo{
                                                   .num_vertices = 3, .num_instances = 1, .first_vertex = 0,
                                                   .first_instance = 0
                                               });
            }
            EngineRenderer::submitGpuCommandBuffer(commandBuffer);
            logger::ILogger::debug("Submitted commandBuffer successfully");
        }

        static void releaseResources(Device *device) {
            EngineRenderer::releaseGpuGraphicsPipeline(device, graphicsPipeline);
            EngineRenderer::releaseGpuBuffer(device, buffer);
        }
    };
}

#endif //AETHERION_KINETICA_RENDER_EXAMPLE_TRIANGLE_H
