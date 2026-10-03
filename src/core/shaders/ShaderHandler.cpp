#include "ShaderHandler.h"

#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>

#include "core/logger/SdlLoggerHandler.h"
#include "internal/GlslToSpirv.h"

#include <SDL3/SDL.h>

#include "spirv_cross.hpp"

namespace {
    using Logger = engine::core::logger::SdlLoggerHandler;
    using namespace engine::core::shaders;

    std::vector<std::uint8_t> readFile(std::string const &fileName) {
        const std::filesystem::path filePath{fileName};
        if (!std::filesystem::exists(filePath)) {
            throw std::runtime_error{std::format("File does not exist: '{}'", fileName)};
        }

        if (!std::filesystem::is_regular_file(filePath)) {
            throw std::runtime_error{std::format("File is invalid: '{}'", fileName)};
        }

        std::ifstream sourceFile(fileName, std::ios::binary);

        if (!sourceFile.good()) {
            throw std::runtime_error{std::format("Failed to open file: '{}'", fileName)};
        }

        return {std::istreambuf_iterator{sourceFile}, {}};
    }

    CompiledShader bytesToCompileShader(const std::uint8_t *data, const std::size_t dataSize) {
        // 1. Calculate how many uint32_t elements fit into the buffer
        const auto element_count{dataSize / sizeof(std::uint32_t)};
        // 2. Allocate the vector buffer upfront
        std::vector<std::uint32_t> result_vec(element_count);
        // 3. Safely copy the raw memory bytes into the vector storage
        std::memcpy(result_vec.data(), data, element_count * sizeof(std::uint32_t));
        return result_vec;
    }

    void storeFile(const CompiledShader &shader, const std::string &fileName) {
        // Open file with std::ios::binary to prevent character translations
        auto file{std::ofstream(fileName, std::ios::out | std::ios::binary)};
        if (!file.is_open()) {
            Logger::error(std::format("Failed to open file: '{}'", fileName));
        }
        // Write the raw buffer directly
        file.write(reinterpret_cast<const char *>(shader.data()),
                   static_cast<std::streamsize>(shader.size() * sizeof(std::uint32_t)));
        Logger::debug(std::format("Successfully stored file '{}'", fileName));
    }

    ShaderInfo identifyShaderInfoInCompiledShader(const CompiledShader &shader) {
        const auto compiler_reflector{spirv_cross::Compiler(shader)};
        const auto resources{compiler_reflector.get_shader_resources()};
        return {
            .num_uniform_buffers = static_cast<std::uint32_t>(resources.uniform_buffers.size()),
            .num_samplers = static_cast<std::uint32_t>(resources.sampled_images.size()),
            .num_storage_bufs = static_cast<std::uint32_t>(resources.storage_buffers.size()),
            .num_storage_texs = static_cast<std::uint32_t>(resources.storage_images.size())
        };
    }

    std::string getShaderEntryPointFromDevice(Device *device) {
        switch (SDL_GetGPUShaderFormats(device)) {
            case SDL_GPU_SHADERFORMAT_MSL:
                return "main0";
            case SDL_GPU_SHADERFORMAT_SPIRV:
                [[fallthrough]];
            case SDL_GPU_SHADERFORMAT_DXIL:
                [[fallthrough]];
            default:
                return "main";
        }
    }

    SDL_GPUShader *createShader(const void *shaderData, const std::size_t shaderSize, const ShaderStage stage,
                                const ShaderInfo &info, Device *device) {
        const auto entryPoint{getShaderEntryPointFromDevice(device)};
        const SDL_GPUShaderCreateInfo gpuShaderinfo = {
            .code_size = shaderSize,
            .code = static_cast<const Uint8 *>(shaderData),
            .entrypoint = entryPoint.c_str(), /* GLSL entry point */
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = static_cast<SDL_GPUShaderStage>(stage),
            .num_samplers = info.num_samplers,
            .num_storage_textures = info.num_storage_texs,
            .num_storage_buffers = info.num_storage_bufs,
            .num_uniform_buffers = info.num_uniform_buffers,
            .props = 0
        };
        return SDL_CreateGPUShader(device, &gpuShaderinfo);
    }
}

ShaderHandler::ShaderHandler() {
    GlslToSpirv::init();
}

CompiledShader ShaderHandler::compileShaderFromSource(const std::string &shaderName,
                                                      const std::string &shaderSource,
                                                      const ShaderKind kind) {
    return GlslToSpirv::compileShader(shaderName, shaderSource, kind);
}

GlslToSpirvHolder ShaderHandler::compileShaderFromFile(const std::string &fileName) {
    const auto sourceShaderBuffer{readFile(fileName)};
    Logger::debug(std::format("Read source from {}, attempting to compile it ...", fileName));
    const auto shaderKind{getShaderKindFromFile(fileName)};
    const std::string sourceShader{std::begin(sourceShaderBuffer), std::end(sourceShaderBuffer)};
    const auto compiledShader{compileShaderFromSource(fileName, sourceShader, shaderKind)};
    return {
        .kind = shaderKind,
        .glslSource = {std::begin(sourceShaderBuffer), std::end(sourceShaderBuffer)},
        .spirvBuffer = compiledShader,
    };
}

void ShaderHandler::storeCompiledShader(const CompiledShader &shader, const std::string &fileName) {
    Logger::debug(std::format("Attempting to store compiled shader in '{}' ...", fileName));
    storeFile(shader, fileName);
}

ShaderKind ShaderHandler::getShaderKindFromFile(const std::string &fileName) {
    if (SDL_strstr(fileName.c_str(), ".vert")) {
        return ShaderKind::VertexShader;
    }
    if (SDL_strstr(fileName.c_str(), ".frag")) {
        return ShaderKind::FragmentShader;
    }
    return ShaderKind::AnyHitShader;
}

ShaderInfo ShaderHandler::getShaderInfoFromSpirv(const CompiledShader &shader) {
    return {identifyShaderInfoInCompiledShader(shader)};
}

Shader *ShaderHandler::loadShader(const GlslToSpirvHolder &shader, Device *device) {
    const auto sdlShader{
        createShader(shader.spirvBuffer.data(), shader.spirvBuffer.size(),
                     shader.stage, shader.info, device)
    };
    if (!sdlShader) {
        Logger::error(std::format("Failed to create shader: {}", SDL_GetError()));
    }
    return sdlShader;
}

Shader *ShaderHandler::loadShaderFromFile(const std::string &fileName, Device *device) {
    std::size_t codeSize{0u};

    const auto code{SDL_LoadFile(fileName.c_str(), &codeSize)};

    if (code == nullptr) {
        Logger::error(std::format("Failed to open file: '{}'", fileName));
        return nullptr;
    }
    const auto stage{
        (SDL_strstr(fileName.c_str(), ".vert") ? ShaderStage::Vertex : ShaderStage::Fragment)
    };

    const auto compiledShaderFromBytes{bytesToCompileShader(static_cast<const std::uint8_t *>(code), codeSize)};
    const auto shaderInfo{identifyShaderInfoInCompiledShader(compiledShaderFromBytes)};
    const auto sdlShader{createShader(code, codeSize, stage, shaderInfo, device)};

    SDL_free(code);

    if (!sdlShader) {
        Logger::error(std::format("Failed to create shader from file '{}' with error: {}", fileName, SDL_GetError()));
    }
    return sdlShader;
}

void ShaderHandler::releaseShader(Shader *shader, Device *device) {
    SDL_ReleaseGPUShader(device, shader);
}
