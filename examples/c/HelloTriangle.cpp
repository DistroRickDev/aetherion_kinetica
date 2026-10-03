#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Vertex layout ── */
typedef struct { float x,y,z, r,g,b,a; } Vertex;

/* ── Load binary file ── */
static void* load_file(const char *path, size_t *sz) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "Cannot open %s\n", path); return NULL; }
    fseek(f, 0, SEEK_END); *sz = (size_t)ftell(f); rewind(f);
    void *d = malloc(*sz); fread(d, 1, *sz, f); fclose(f);
    return d;
}

/* ── Create shader from SPIR-V file ── */
static SDL_GPUShader* make_shader(
    SDL_GPUDevice *dev, const char *path,
    SDL_GPUShaderStage stage, Uint32 samplers, Uint32 ubos
) {
    size_t sz; void *code = load_file(path, &sz);
    SDL_GPUShaderCreateInfo info = {
        .code = code, .code_size = sz,
        .entrypoint          = "main",
        .format              = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage               = stage,
        .num_samplers        = samplers,
        .num_uniform_buffers = ubos,
    };
    SDL_GPUShader *s = SDL_CreateGPUShader(dev, &info);
    free(code); return s;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    /* ── Init ── */
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *win = SDL_CreateWindow("Hello Triangle", 800, 600, 0);

    SDL_GPUDevice *dev = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV, true /*debug*/, NULL
    );
    SDL_ClaimWindowForGPUDevice(dev, win);

    /* ── Vertex data ── */
    Vertex verts[] = {
        {  0.0f,  0.5f, 0.0f,   1,0,0,1 },
        { -0.5f, -0.5f, 0.0f,   0,1,0,1 },
        {  0.5f, -0.5f, 0.0f,   0,0,1,1 },
    };

    /* ── GPU vertex buffer ── */
    SDL_GPUBuffer *vb = SDL_CreateGPUBuffer(dev, &(SDL_GPUBufferCreateInfo){
        .usage=SDL_GPU_BUFFERUSAGE_VERTEX, .size=sizeof(verts)
    });

    /* ── Upload vertices ── */
    SDL_GPUTransferBuffer *stg = SDL_CreateGPUTransferBuffer(dev, &(SDL_GPUTransferBufferCreateInfo){
        .usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size=sizeof(verts)
    });
    void *ptr = SDL_MapGPUTransferBuffer(dev, stg, false);
    memcpy(ptr, verts, sizeof(verts));
    SDL_UnmapGPUTransferBuffer(dev, stg);
    {
        SDL_GPUCommandBuffer *c = SDL_AcquireGPUCommandBuffer(dev);
        SDL_GPUCopyPass      *p = SDL_BeginGPUCopyPass(c);
        SDL_UploadToGPUBuffer(p,
            &(SDL_GPUTransferBufferLocation){.transfer_buffer=stg,.offset=0},
            &(SDL_GPUBufferRegion){.buffer=vb,.offset=0,.size=sizeof(verts)},
            false);
        SDL_EndGPUCopyPass(p);
        SDL_SubmitGPUCommandBuffer(c);
        SDL_WaitForGPUIdle(dev);
    }
    SDL_ReleaseGPUTransferBuffer(dev, stg);

    /* ── Shaders ── */
    SDL_GPUShader *vs = make_shader(dev, "vert.spv", SDL_GPU_SHADERSTAGE_VERTEX,   0, 0);
    SDL_GPUShader *fs = make_shader(dev, "frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0);

    /* ── Pipeline ── */
    SDL_GPUVertexAttribute attrs[] = {
        { .location=0, .binding=0, .format=SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset=0  },
        { .location=1, .binding=0, .format=SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset=12 },
    };
    SDL_GPUVertexBinding bindings[] = {{
        .binding=0, .pitch=sizeof(Vertex), .input_rate=SDL_GPU_VERTEXINPUTRATE_VERTEX
    }};
    SDL_GPUColorTargetDescription ctarget = {
        .format=SDL_GetGPUSwapchainTextureFormat(dev, win)
    };
    SDL_GPUGraphicsPipeline *pipe = SDL_CreateGPUGraphicsPipeline(dev,
        &(SDL_GPUGraphicsPipelineCreateInfo){
            .vertex_shader  = vs,
            .fragment_shader= fs,
            .vertex_input_state = {
                .vertex_bindings       = bindings, .num_vertex_bindings  = 1,
                .vertex_attributes     = attrs,    .num_vertex_attributes= 2,
            },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .rasterizer_state    = { .fill_mode=SDL_GPU_FILLMODE_FILL, .cull_mode=SDL_GPU_CULLMODE_NONE },
            .target_info         = { .color_target_descriptions=&ctarget, .num_color_targets=1 },
        }
    );
    SDL_ReleaseGPUShader(dev, vs);
    SDL_ReleaseGPUShader(dev, fs);

    /* ── Main loop ── */
    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev))
            if (ev.type == SDL_EVENT_QUIT) running = false;

        SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(dev);
        SDL_GPUTexture *sc; Uint32 w, h;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, win, &sc, &w, &h)) {
            SDL_CancelGPUCommandBuffer(cmd); continue;
        }

        SDL_GPURenderPass *rp = SDL_BeginGPURenderPass(cmd,
            &(SDL_GPUColorTargetInfo){
                .texture     = sc,
                .load_op     = SDL_GPU_LOADOP_CLEAR,
                .store_op    = SDL_GPU_STOREOP_STORE,
                .clear_color = {.r=.05f, .g=.05f, .b=.1f, .a=1},
            }, 1, NULL);

        SDL_BindGPUGraphicsPipeline(rp, pipe);
        SDL_BindGPUVertexBuffers(rp, 0,
            &(SDL_GPUBufferBinding){.buffer=vb, .offset=0}, 1);
        SDL_DrawGPUPrimitives(rp, 3, 1, 0, 0);
        SDL_EndGPURenderPass(rp);
        SDL_SubmitGPUCommandBuffer(cmd);
    }

    /* ── Cleanup ── */
    SDL_WaitForGPUIdle(dev);
    SDL_ReleaseGPUGraphicsPipeline(dev, pipe);
    SDL_ReleaseGPUBuffer(dev, vb);
    SDL_ReleaseWindowFromGPUDevice(dev, win);
    SDL_DestroyGPUDevice(dev);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}