#include "core/render/EngineRenderer.h"

#include "core/logger/SdlLoggerHandler.h"
#include "core/shaders/ShaderHandler.h"

#include <cstring>
#include <format>
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

    SDL_GPUShader *createShader(const void *shaderData, const std::size_t shaderSize, const std::uint32_t stage,
                                const std::string &entryPoint,
                                const std::uint32_t sampled_images_num, const std::uint32_t storage_images_num,
                                const std::uint32_t storage_buffers_num, const std::uint32_t uniform_buffers_num,
                                GpuDevice *device) {
        const SDL_GPUShaderCreateInfo gpuShaderInfo = {
            .code_size = shaderSize,
            .code = static_cast<const Uint8 *>(shaderData),
            .entrypoint = entryPoint.c_str(), /* GLSL entry point */
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = static_cast<SDL_GPUShaderStage>(stage),
            .num_samplers = sampled_images_num,
            .num_storage_textures = storage_images_num,
            .num_storage_buffers = storage_buffers_num,
            .num_uniform_buffers = uniform_buffers_num,
            .props = 0
        };
        return SDL_CreateGPUShader(device, &gpuShaderInfo);
    }

    shaders::GlslToSpirvHolder getShaderHolderFromFile(const std::string &fileName) {
        if (!shaders::ShaderHandler::isShaderFileSpirvType(fileName)) {
            Logger::debug("Shader file type is not SPIR-V, attempting to compile it");
            return shaders::ShaderHandler::compileShaderFromFile(fileName);
        }
        Logger::debug("Shader file type is SPIR-V");
        return shaders::ShaderHandler::loadCompiledShaderFromFile(fileName);
    }

    SDL_GPUVertexElementFormat getSdlVertexFormat(const spirv_cross::SPIRType &type) {
        if (type.basetype == spirv_cross::SPIRType::Float) {
            switch (type.vecsize) {
                case 1: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
                case 2: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
                case 3: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
                case 4: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
                default: return SDL_GPU_VERTEXELEMENTFORMAT_INVALID;;
            }
        }
        if (type.basetype == spirv_cross::SPIRType::Int) {
            switch (type.vecsize) {
                case 1: return SDL_GPU_VERTEXELEMENTFORMAT_INT;
                case 2: return SDL_GPU_VERTEXELEMENTFORMAT_INT2;
                case 3: return SDL_GPU_VERTEXELEMENTFORMAT_INT3;
                case 4: return SDL_GPU_VERTEXELEMENTFORMAT_INT4;
                default: return SDL_GPU_VERTEXELEMENTFORMAT_INVALID;;
            }
        }
        return SDL_GPU_VERTEXELEMENTFORMAT_INVALID;
    }

    std::uint32_t getVertexFormatSize(const SDL_GPUVertexElementFormat format) {
        switch (format) {
            case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT: return 4;
            case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2: return 8;
            case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3: return 12;
            case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4: return 16;
            case SDL_GPU_VERTEXELEMENTFORMAT_INT: return 4;
            case SDL_GPU_VERTEXELEMENTFORMAT_INT2: return 8;
            case SDL_GPU_VERTEXELEMENTFORMAT_INT3: return 12;
            case SDL_GPU_VERTEXELEMENTFORMAT_INT4: return 16;
            default: return 0;
        }
    }

    struct GpuVertexAttributeResult {
        std::vector<SDL_GPUVertexAttribute> vertex_attribute_vectors;
        SDL_GPUVertexBufferDescription vertex_buffer_description;
        GpuShader *vertexShader;
    };

    GpuVertexAttributeResult reflectVertexAttributesFromShader(const shaders::CompiledShader &vertexShader,
                                                               const std::string &fileName, GpuDevice *device) {
        Logger::debug("Obtaining vertex shader compiler reflector...");
        const auto vertexShaderCompilerReflector{
            shaders::ShaderHandler::getShaderCompilerReflectorFromCompiledShader(vertexShader)
        };
        Logger::debug("Obtained vertex shader compiler reflector...");
        const auto resources{vertexShaderCompilerReflector.get_shader_resources()};
        std::vector<SDL_GPUVertexAttribute> vertexAttributes{};
        vertexAttributes.reserve(resources.stage_inputs.size());
        auto current_offset{0u};
        for (const auto &resource: resources.stage_inputs) {
            const auto location{vertexShaderCompilerReflector.get_decoration(resource.id, spv::DecorationLocation)};
            const auto format{getSdlVertexFormat(vertexShaderCompilerReflector.get_type(resource.type_id))};
            vertexAttributes.push_back(
                {
                    .location = location,
                    .buffer_slot = 0,
                    .format = format,
                    .offset = current_offset
                }
            );
            current_offset += getVertexFormatSize(format);
        }

        std::ranges::sort(vertexAttributes, [](const auto &attr1, const auto &attr2) {
            return attr1.location < attr2.location;
        });

        const auto gpuVertexShader{
            createShader(
                vertexShader.data(),
                vertexShader.size() * sizeof(std::uint32_t),
                static_cast<std::uint32_t>(shaders::ShaderStage::Vertex),
                shaders::ShaderHandler::getShaderEntryPointFromSourceFileName(fileName),
                resources.sampled_images.size(),
                resources.storage_images.size(),
                resources.storage_buffers.size(),
                resources.uniform_buffers.size(),
                device
            )
        };

        return {
            .vertex_attribute_vectors = vertexAttributes,
            .vertex_buffer_description = {
                .slot = 0, .pitch = current_offset, .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                .instance_step_rate = 0
            },
            .vertexShader = gpuVertexShader
        };
    }

    struct FragmentReflectionResult {
        std::uint32_t sampler_count;
        std::vector<std::uint32_t> active_sampler_bindings;
        GpuShader *fragmentShader;
    };

    FragmentReflectionResult reflectFragmentSamplersFromShader(const shaders::CompiledShader &fragmentShader,
                                                               const std::string &fileName, GpuDevice *device) {
        Logger::debug("Obtaining fragment shader compiler reflector...");
        const auto fragmentShaderCompilerReflector{
            shaders::ShaderHandler::getShaderCompilerReflectorFromCompiledShader(fragmentShader)
        };
        Logger::debug("Obtained fragment shader compiler reflector...");
        const auto resources{fragmentShaderCompilerReflector.get_shader_resources()};

        FragmentReflectionResult result{};
        auto maxBindingIndex{0u};
        auto hasSamplers{false};

        for (const auto &resource: resources.sampled_images) {
            const auto set{fragmentShaderCompilerReflector.get_decoration(resource.id, spv::DecorationDescriptorSet)};
            const auto binding{fragmentShaderCompilerReflector.get_decoration(resource.id, spv::DecorationBinding)};

            if (2 == set) {
                hasSamplers = true;
                result.active_sampler_bindings.push_back(binding);
                if (binding > maxBindingIndex) {
                    maxBindingIndex = binding;
                }
            }
        }
        result.sampler_count = hasSamplers ? maxBindingIndex + 1 : 0;
        std::ranges::sort(result.active_sampler_bindings);

        result.fragmentShader = createShader(fragmentShader.data(),
                                             fragmentShader.size() * sizeof(std::uint32_t),
                                             static_cast<std::uint32_t>(shaders::ShaderStage::Fragment),
                                             shaders::ShaderHandler::getShaderEntryPointFromSourceFileName(fileName),
                                             resources.sampled_images.size(),
                                             resources.storage_images.size(),
                                             resources.storage_buffers.size(),
                                             resources.uniform_buffers.size(),
                                             device);
        return result;
    }
}

GpuGraphicsPipeline *render::EngineRenderer::createGpuGraphicsPipeline(const std::string &vertexShaderFileName,
                                                                       const std::string &fragmentShaderFileName,
                                                                       GpuDevice *device, Window *window) {
    const auto vertexShaderHolder{getShaderHolderFromFile(vertexShaderFileName)};
    const auto fragmentShaderHolder{getShaderHolderFromFile(fragmentShaderFileName)};

    const auto [vertex_attribute_vectors, vertex_buffer_description, vertexShader]{
        reflectVertexAttributesFromShader(vertexShaderHolder.spirvBuffer, vertexShaderFileName, device)
    };
    const auto fragmentAttributes{
        reflectFragmentSamplersFromShader(fragmentShaderHolder.spirvBuffer, fragmentShaderFileName, device)
    };

    auto vertexInputState{
        SDL_GPUVertexInputState{
            .vertex_buffer_descriptions = &vertex_buffer_description,
            .num_vertex_buffers = 1,
            .vertex_attributes = vertex_attribute_vectors.data(),
            .num_vertex_attributes = static_cast<std::uint32_t>(vertex_attribute_vectors.size())
        }
    };

    auto colorTargetDescription{
        SDL_GPUColorTargetDescription{
            .format = SDL_GetGPUSwapchainTextureFormat(device, window),

        }
    };

    auto pipelineCreateInfo{
        SDL_GPUGraphicsPipelineCreateInfo{
            .vertex_shader = vertexShader,
            .fragment_shader = fragmentAttributes.fragmentShader,
            .vertex_input_state = vertexInputState,
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,

            .rasterizer_state = {
                .fill_mode = SDL_GPU_FILLMODE_FILL,
                .cull_mode = SDL_GPU_CULLMODE_BACK,
                .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
            },

            .target_info = {
                .color_target_descriptions = &colorTargetDescription,
                .num_color_targets = 1,
                .has_depth_stencil_target = false
            }
        }
    };

    const auto result{SDL_CreateGPUGraphicsPipeline(device, &pipelineCreateInfo)};
    if (!result) {
        throw std::runtime_error("Failed to create GPU GraphicsPipeline");
    }
    SDL_ReleaseGPUShader(device, vertexShader);
    SDL_ReleaseGPUShader(device, fragmentAttributes.fragmentShader);
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

    const auto textureSamplerBinding{SDL_GPUTextureSamplerBinding{
        
    }};

    SDL_BindGPUFragmentSamplers(renderPass, 1, &textureSamplerBinding, 1);
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
    SDL_UnmapGPUTransferBuffer(device, stg);
    {
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
