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

    constexpr auto kTestShaderPath{"/home/ditrorickdev/projects/aetherion_kinetica/examples/shaders/glsl"};
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
            const auto vertexShader = shaders::ShaderHandler::loadShaderFromFile(
                std::format("{}/{}", kTestShaderPath, "shader.vert.spirv"), device);
            const auto fragmentShader = shaders::ShaderHandler::loadShaderFromFile(
                std::format("{}/{}", kTestShaderPath, "shader.frag.spirv"), device);
            if (!vertexShader || !fragmentShader) {
                throw std::runtime_error("Failed to load shaders");
            }
            logger::ILogger::debug("Vertex shader and fragment shaders created successfully");
            graphicsPipeline = EngineRenderer::createGpuGraphicsPipeline(
                vertexShader, fragmentShader, device, window);
            logger::ILogger::debug("Graphics Pipeline created successfully");
            shaders::ShaderHandler::releaseShader(vertexShader, device);
            shaders::ShaderHandler::releaseShader(fragmentShader, device);
            logger::ILogger::debug("Vertex shader and fragment shaders released successfully");
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
