#include "GlslToSpirv.h"

#include <format>
#include <memory>

#include "core/logger/SdlLoggerHandler.h"
#include "shaderc/shaderc.hpp"

namespace {
    using namespace engine::core::shaders;

    struct SpirvVersion {
        std::uint32_t version{0u};
        std::uint32_t revision{0u};
    };

    const auto kVersion{std::make_unique<SpirvVersion>()};
    const auto kSpvCompiler{std::make_unique<shaderc::Compiler>()};
    const auto kSpvCompileOptions{std::make_unique<shaderc::CompileOptions>()};
}

void GlslToSpirv::init(const bool optimize) {
    shaderc_get_spv_version(&kVersion->version, &kVersion->revision);
    if (optimize) {
        kSpvCompileOptions->SetOptimizationLevel(shaderc_optimization_level_performance);
    }
    logger::SdlLoggerHandler::debug(std::format("GlslToSpirv initialized shaderc version: {}, revision: {}",
                                                kVersion->version, kVersion->revision));
}

std::vector<std::uint32_t> GlslToSpirv::compileShader(const std::string &source_name, const std::string &source,
                                                      const ShaderKind kind) {
    if (!kSpvCompiler) {
        throw std::runtime_error("shaderc compiler not initialized");
    }
    const auto module{
        kSpvCompiler->CompileGlslToSpv(source, static_cast<shaderc_shader_kind>(kind),
                                       source_name.c_str(), *kSpvCompileOptions)
    };
    if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
        logger::SdlLoggerHandler::debug(std::format(
            "Failed to compiled GLSL to SPIR-V with error: {}",
            module.GetErrorMessage()));
        return {};
    }
    logger::SdlLoggerHandler::debug(std::format("Successfully compiled {}", source_name));
    return {std::begin(module), std::end(module)};
}

std::string GlslToSpirv::preProcessShader(const std::string &source_name, const std::string &source,
                                          const ShaderKind kind) {
    if (!kSpvCompiler) {
        throw std::runtime_error("shaderc compiler not initialized");
    }

    const auto module{
        kSpvCompiler->PreprocessGlsl(source, static_cast<shaderc_shader_kind>(kind),
                                     source_name.c_str(), *kSpvCompileOptions)
    };
    if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
        logger::SdlLoggerHandler::debug(std::format("Failed to preprocess GLSL with error: {}",
                                                    module.GetErrorMessage()));
        return {};
    }
    logger::SdlLoggerHandler::debug(std::format("Successfully preprocessed {}", source_name));
    return {std::begin(module), std::end(module)};
}
