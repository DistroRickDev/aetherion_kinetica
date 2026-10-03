#version 450
#pragma shader_stage(vertex)

/* Input attributes — match SDL_GPUVertexAttribute layout */
layout(location = 0) in  vec3 in_position;
layout(location = 1) in  vec4 in_color;
layout(location = 2) in  vec2 in_uv;

/* Outputs to the fragment shader */
layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec2 frag_uv;

/* Uniform buffer at set=1, binding=0 (SDL GPU convention) */
layout(set = 1, binding = 0) uniform TransformUBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_position, 1.0);
    frag_color  = in_color;
    frag_uv     = in_uv;
}
