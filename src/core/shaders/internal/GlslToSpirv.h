#ifndef AETHERION_KINETICA_GLSL_TO_SPIRV_H
#define AETHERION_KINETICA_GLSL_TO_SPIRV_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>


namespace engine::core::shaders {
    enum class ShaderKind : std::uint8_t {
        // Forced shader kinds. These shader kinds force the compiler to compile the
        // source code as the specified kind of shader.
        VertexShader = 0,
        FragmentShader,
        ComputeShader,
        GeometryShader,
        TessControlShader,
        TessEvaluationShader,

        GlslVertexShader = VertexShader,
        GlslFragmentShader = FragmentShader,
        GlslComputeShader = ComputeShader,
        GlslGeometryShader = GeometryShader,
        GlslTessControlShader = TessControlShader,
        GlslTessEvaluationShader = TessEvaluationShader,


        // Deduce the shader kind from #pragma annotation in the source code. Compiler
        // will emit error if #pragma annotation is not found.
        InferFromSource,
        // Default shader kinds. Compiler will fall back to compile the source code as
        // the specified kind of shader when #pragma annotation is not found in the
        // source code.
        GlslDefaultVertexShader,
        GlslDefaultFragmentShader,
        GlslDefaultComputeShader,
        GlslDefaultGeometryShader,
        GlslDefaultTessControlShader,
        GlslDefaultTessEvaluationShader,

        SpirvAssembly,
        RaygenShader,
        AnyHitShader,
        ClosestHitShader,
        MissShader,
        IntersectionShader,
        CallableShader,
        GlslRaygenShader = RaygenShader,
        GlslAnyHitShader = AnyHitShader,
        GlslClosestHitShader = ClosestHitShader,
        GlslMissShader = MissShader,
        GlslIntersectionShader = IntersectionShader,
        GlslCallableShader = CallableShader,

        GlslDefaultRaygenShader,
        GlslDefaultAnyHitShader,
        GlslDefaultClosestHitShader,
        GlslDefaultMissShader,
        GlslDefaultIntersectionShader,
        GlslDefaultCallableShader,

        TaskShader,
        MeshShader,
        GlslTaskShader = TaskShader,
        GlslMeshShader = MeshShader,
        GlslDefaultTaskShader,
        GlslDefaultMeshShader
    };

    struct GlslToSpirv {
        ~GlslToSpirv() = default;

        static void init(bool optimize = true);

        static std::vector<std::uint32_t> compileShader(const std::string &source_name, const std::string &source,
                                                        ShaderKind kind);

        static std::string preProcessShader(const std::string &source_name, const std::string &source,
                                            ShaderKind kind);
    };
}

#endif //AETHERION_KINETICA_GLSL_TO_SPIRV_H
