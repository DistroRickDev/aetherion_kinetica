#ifndef AETHERION_KINETICA_SHADER_HANDLER_H
#define AETHERION_KINETICA_SHADER_HANDLER_H

#include <string>
#include <vector>

#include "internal/GlslToSpirv.h"

class SDL_GPUShader;
class SDL_GPUDevice;

using Shader = SDL_GPUShader;
using Device = SDL_GPUDevice;

namespace engine::core::shaders {
    using CompiledShader = std::vector<std::uint32_t>;

    enum class ShaderStage : std::uint8_t {
        Vertex = 0u,
        Fragment = 1u,
    };

    struct ShaderInfo {
        std::uint32_t num_uniform_buffers{0u};
        std::uint32_t num_samplers{0u};
        std::uint32_t num_storage_bufs{0u};
        std::uint32_t num_storage_texs{0u};
    } __attribute__((packed));

    struct GlslToSpirvHolder {
        ShaderKind kind;
        std::string glslSource{};
        CompiledShader spirvBuffer{};
        ShaderInfo info{};
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

        static ShaderInfo getShaderInfoFromSpirv(const CompiledShader& shader);

        static Shader* loadShader(const GlslToSpirvHolder &shader, Device *device);

        static Shader* loadShaderFromFile(const std::string &fileName, Device *device);

        static void releaseShader(Shader *shader, Device *device);
    };
}

#endif //AETHERION_KINETICA_SHADER_HANDLER_H
