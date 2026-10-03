#version 450
#pragma shader_stage(fragment)

/* 1. Inputs: These must match the OUT variables of your vertex shader exactly */
layout(location = 0) in vec4 frag_color;
layout(location = 1) in vec2 frag_uv;

/* 2. Output: The final color written to your SDL_GPU render target */
layout(location = 0) out vec4 out_color;

/* 3. Optional: If you want to sample a texture, un-comment this block */
layout(set = 2, binding = 0) uniform sampler2D tex_sampler;

void main() {
    // Just output the interpolated vertex colors directly to the screen
    // out_color = frag_color;

    // Alternative: If you are texturing, un-comment this line instead:
    out_color = texture(tex_sampler, frag_uv) * frag_color;
}