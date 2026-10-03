#ifndef AETHERION_KINETICA_SHADER_HANDLER_H
#define AETHERION_KINETICA_SHADER_HANDLER_H

#include <string>
#include <vector>

#include "internal/GlslToSpirv.h"
#include "spirv_cross.hpp"

namespace engine::core::shaders {
    using CompiledShader = std::vector<std::uint32_t>;

    enum class ShaderStage : std::uint8_t {
        Vertex = 0u,
        Fragment = 1u,
    };

    struct GlslToSpirvHolder {
        ShaderKind kind;
        std::string glslSource{};
        CompiledShader spirvBuffer{};
        ShaderStage stage{};
    };

    class ShaderHandler {
    public:
        ShaderHandler();

        ShaderHandler(ShaderHandler const &) = delete;

        void operator=(ShaderHandler const &) = delete;

        ~ShaderHandler() = default;

        static CompiledShader compileShaderFromSource(const std::string &shaderName, const std::string &shaderSource,
                             ShaderKind kind = ShaderKind::InferFromSource);

        static GlslToSpirvHolder compileShaderFromFile(const std::string &fileName);

        static void storeCompiledShader(const CompiledShader &shader, const std::string &fileName);

        static ShaderKind getShaderKindFromFile(const std::string &fileName);

        static ShaderStage getShaderStage(const std::string &fileName);

        static spirv_cross::Compiler getShaderCompilerReflectorFromCompiledShader(const CompiledShader& shader);

        static std::string getShaderEntryPointFromSourceFileName(const std::string &fileName);

        static GlslToSpirvHolder loadCompiledShaderFromFile(const std::string &fileName);

        static bool isShaderFileSpirvType(const std::string &fileName);

    };
}

#endif //AETHERION_KINETICA_SHADER_HANDLER_H
