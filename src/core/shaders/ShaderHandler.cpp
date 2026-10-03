#include "ShaderHandler.h"

#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>

#include "core/logger/SdlLoggerHandler.h"
#include "internal/GlslToSpirv.h"
#include "SDL3/SDL.h"

namespace {
    using Logger = engine::core::logger::SdlLoggerHandler;
    using namespace engine::core::shaders;

    CompiledShader bytesToCompileShader(const std::uint8_t *data, const std::size_t dataSize) {
        // 1. Calculate how many uint32_t elements fit into the buffer
        const auto element_count{dataSize / sizeof(std::uint32_t)};
        // 2. Allocate the vector buffer upfront
        std::vector<std::uint32_t> result_vec(element_count);
        // 3. Safely copy the raw memory bytes into the vector storage
        std::memcpy(result_vec.data(), data, element_count * sizeof(std::uint32_t));
        return result_vec;
    }

    CompiledShader readCompiledShaderFile(std::string const &fileName) {
        std::size_t codeSize{0u};
        const auto code{SDL_LoadFile(fileName.c_str(), &codeSize)};
        if (code == nullptr) {
            Logger::error(std::format("Failed to open file: '{}'", fileName));
        }
        const auto res{bytesToCompileShader(static_cast<const std::uint8_t *>(code), codeSize)};

        SDL_free(code);
        return res;
    }

    std::vector<std::uint8_t> readSourceFile(std::string const &fileName) {
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
    const auto sourceShaderBuffer{readSourceFile(fileName)};
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

ShaderStage ShaderHandler::getShaderStage(const std::string &fileName) {
    if (SDL_strstr(fileName.c_str(), ".vert")) {
        return ShaderStage::Vertex;
    }
    return ShaderStage::Fragment;
}

spirv_cross::Compiler ShaderHandler::getShaderCompilerReflectorFromCompiledShader(const CompiledShader &shader) {
    Logger::debug(std::format("Attempting to use compiled shader with size '{}' ...", shader.size()));
    return spirv_cross::Compiler(shader);
}

std::string ShaderHandler::getShaderEntryPointFromSourceFileName(const std::string &fileName) {
    if (SDL_strstr(fileName.c_str(), ".glsl") || SDL_strstr(fileName.c_str(), ".spv")) {
        return "main";
    }
    return "";
}

GlslToSpirvHolder ShaderHandler::loadCompiledShaderFromFile(const std::string &fileName) {
    return {
        .kind = getShaderKindFromFile(fileName),
        .glslSource = {},
        .spirvBuffer = readCompiledShaderFile(fileName),
        .stage = getShaderStage(fileName)
    };
}

bool ShaderHandler::isShaderFileSpirvType(const std::string &fileName) {
    const std::filesystem::path filePath{fileName};
    return std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath) && SDL_strstr(fileName.c_str(), ".spv");
}
