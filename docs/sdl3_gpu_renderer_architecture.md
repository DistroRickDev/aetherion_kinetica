# Complete Architectural Guide: Modern GPU Programming & Custom Renderer Design with SDL3 GPU

This guide is a comprehensive, end-to-end conceptual blueprint for designing and building a custom 2D/3D game engine renderer using the **SDL3 GPU API (`SDL_GPU`)**. It bridges low-level graphics hardware theory with SDL3's modern, cross-platform abstraction over **Vulkan, Direct3D 12, and Metal**.

---

## Table of Contents

1. [Part I: Hardware Foundations & The SDL3 GPU Execution Model](#part-i-hardware-foundations--the-sdl3-gpu-execution-model)
2. [Part II: CPU–GPU Synchronization, Memory & Frame Pacing](#part-ii-cpugpu-synchronization-memory--frame-pacing)
3. [Part III: Geometry, 3D Coordinate Spaces & Data Delivery](#part-iii-geometry-3d-coordinate-spaces--data-delivery)
4. [Part IV: Textures, Samplers & Physically Based Shading (PBR)](#part-iv-textures-samplers--physically-based-shading-pbr)
5. [Part V: Visibility, Culling, LOD & GPU-Driven Scaling](#part-v-visibility-culling-lod--gpu-driven-scaling)
6. [Part VI: Multi-Pass Architectures, Shadows, HDR & Anti-Aliasing](#part-vi-multi-pass-architectures-shadows-hdr--anti-aliasing)
7. [Part VII: Compute Shaders & GPU Profiling/Debugging](#part-vii-compute-shaders--gpu-profilingdebugging)
8. [Part VIII: End-to-End Engine Frame Blueprint](#part-viii-end-to-end-engine-frame-blueprint)

---

## Part I: Hardware Foundations & The SDL3 GPU Execution Model

### 1. Two Separate Worlds: Host (CPU) vs. Device (GPU)
A computer running a real-time renderer operates as two distinct processors connected over a relatively slow bus (PCIe):
* **The CPU (Host):** Optimized for low-latency serial execution, complex branching, game logic, physics orchestration, and file I/O.
* **The GPU (Device):** A massively parallel throughput processor with thousands of smaller cores designed to execute identical mathematical operations across millions of vertices and pixels simultaneously.
* **Separate Memory Pools:** The CPU accesses system **RAM**, while the GPU operates on high-bandwidth **VRAM**. The CPU cannot directly dereference or mutate VRAM pointers mid-draw; instead, data is staged in shared memory and transferred explicitly via GPU copy commands.

### 2. The Hardware Graphics Pipeline
Transforming 3D world geometry into a 2D image follows a strict, mostly one-way assembly line:
1. **Input Assembly:** Reads raw vertex and index buffers and groups vertices into geometric primitives (triangles, lines, points).
2. **Vertex Shader (Programmable):** Executes once per vertex to transform 3D model coordinates into normalized screen space and forward attributes (UVs, normals, colors).
3. **Clipping & Viewport Transform (Fixed Hardware):** Discards primitives outside the view volume and maps coordinates to exact pixel dimensions.
4. **Rasterization (Fixed Hardware):** Connects transformed vertices into triangles and determines which screen pixels (fragments) lie inside each triangle, interpolating vertex attributes across the surface.
5. **Fragment / Pixel Shader (Programmable):** Executes once per covered pixel to sample textures, evaluate lighting equations, and output final color values.
6. **Output Merger (Fixed Hardware):** Performs **Depth/Stencil Testing** and **Color Blending**, writing surviving pixels into active Render Targets.

### 3. Why SDL3 GPU Exists
Graphics APIs historically forced a trade-off between simplicity and hardware control:
* **Legacy APIs (OpenGL):** Hidden driver state machines caused unpredictable stalls, poor multithreading, and limited access to modern features.
* **Explicit Native APIs (Vulkan, Direct3D 12, Metal):** Full hardware control at the cost of thousands of lines of platform-specific boilerplate for descriptor pools, memory allocators, and manual pipeline barriers.
* **The SDL3 GPU Sweet Spot:** Exposes the modern **command-buffer and pre-baked pipeline** architecture natively across Windows, Linux, macOS, iOS, and consoles while automating hazard tracking, memory barriers, swapchain lifecycles, and resource destruction.

### 4. Core SDL3 GPU Architectural Pillars
* **GPU Device (`SDL_GPUDevice`):** Your logical handle to the physical graphics adapter, used to allocate buffers, textures, samplers, shaders, and pipelines.
* **Window & Swapchain (`SDL_ClaimWindowForGPUDevice`):** Attaches a rotating ring of presentable backbuffers (the **Swapchain**) to an `SDL_Window`. While the monitor scans out one buffer, the GPU renders into the next.
* **Command Buffers (`SDL_GPUCommandBuffer`):** Recording clipboards acquired each frame. You record all Copy, Compute, and Render passes into a command buffer and submit it asynchronously to the GPU queue.
* **Explicit Passes:** All GPU execution occurs inside isolated passes:
  * **Copy Pass (`SDL_GPUCopyPass`):** Transfers data between CPU-visible **Transfer Buffers** and high-speed VRAM buffers/textures.
  * **Compute Pass (`SDL_GPUComputePass`):** Dispatches general-purpose parallel compute workloads.
  * **Render Pass (`SDL_GPURenderPass`):** Binds Color and Depth/Stencil Render Targets, clears or loads them, and executes draw calls.
* **Immutable Pipelines (`SDL_GPUGraphicsPipeline` / `SDL_GPUComputePipeline`):** Combines compiled shaders, vertex input layouts, rasterizer culling modes, depth/stencil rules, blend equations, and target pixel formats into a single pre-validated state object.
* **Standardized Resource Binding Sets (Sets 0–3):** Eliminates manual descriptor pool management by enforcing a fixed binding layout:
  * **Set 0:** Vertex shader sampled textures, storage textures, and storage buffers.
  * **Set 1:** Vertex shader uniform buffers (fast push-style constants).
  * **Set 2:** Fragment shader sampled textures, storage textures, and storage buffers.
  * **Set 3:** Fragment shader uniform buffers (fast push-style constants).
* **Ahead-of-Time Shaders (`SDL_shadercross`):** `SDL_GPU` ingests compiled backend bytecode (**SPIR-V** for Vulkan, **DXIL** for D3D12, **MSL** for Metal). Using **SDL_shadercross**, your asset pipeline can compile HLSL or SPIR-V into all target formats offline or at runtime.

---

## Part II: CPU–GPU Synchronization, Memory & Frame Pacing

### 1. Asynchronous Relay Execution & Frames in Flight
When `SDL_SubmitGPUCommandBuffer()` returns, the GPU has not yet executed those commands—they are queued.
* **Overlap:** While the GPU renders **Frame N**, the CPU simulates game logic and records commands for **Frame N+1**.
* **Frames in Flight:** Engines cap queued frames to **2 or 3** rotating slots. If the CPU runs faster than the GPU, it blocks briefly before starting a frame whose slot is still busy, preventing multi-frame input lag.
* **Automatic Throttling in SDL3:** Calling `SDL_WaitAndAcquireGPUSwapchainTexture()` automatically paces the CPU against the swapchain queue without requiring manual fence management.

### 2. Automatic Hazard Tracking & Transfer Buffer Cycling
* **Inter-Pass Barriers:** SDL3 tracks read and write usage of every buffer and texture across Copy, Compute, and Render passes, automatically inserting hardware pipeline barriers so a pass never reads half-written data from an earlier pass.
* **Transfer Buffer Cycling (`cycle = true`):** Updating dynamic data (such as per-frame particles or UI quads) into a Transfer Buffer while the GPU is still copying last frame's data causes race conditions. Passing `cycle = true` to `SDL_MapGPUTransferBuffer()` tells SDL3 to automatically allocate or reuse an idle internal memory block if the current one is still in flight.
* **Deferred Resource Destruction:** Calling `SDL_ReleaseGPUTexture()` or `SDL_ReleaseGPUBuffer()` safely defers freeing VRAM until all submitted command buffers referencing that resource finish executing.

### 3. Swapchain Present Modes (`SDL_GPUPresentMode`)

| Present Mode | Behavior | Screen Tearing | Input Latency | Best Use Case |
| :--- | :--- | :--- | :--- | :--- |
| **V-Sync (`VSYNC`)** | Waits for the display's vertical blanking interval before swapping. | **None** | Moderate | Standard single-player games, power-constrained devices |
| **Immediate (`IMMEDIATE`)** | Swaps to the screen the instant rendering finishes. | **Yes** | **Lowest** | Competitive esports titles prioritizing raw response time |
| **Mailbox (`MAILBOX`)** | Fast triple-buffering; newer frames overwrite unshown queued frames. | **None** | Low | High-FPS desktop rendering without tearing |

---

## Part III: Geometry, 3D Coordinate Spaces & Data Delivery

### 1. Vertex Buffers, Index Buffers & Mesh Optimization
* **Vertex Buffers (`SDL_GPU_BUFFERUSAGE_VERTEX`):** Store per-vertex attributes (position, normal, tangent, UVs, bone weights).
* **Index Buffers (`SDL_GPU_BUFFERUSAGE_INDEX`):** Store compact 16-bit or 32-bit integer indices referencing shared vertices so adjacent triangles avoid duplicating vertex data.
* **Post-Transform Vertex Cache Optimization:** Reordering indices so consecutive triangles share recently transformed vertices maximizes hardware vertex cache hits.
* **Vertex Compression:** Packing normals into `10_10_10_2` normalized integers, UVs into `FLOAT16`, and vertex colors into `UBYTE4_NORM` dramatically cuts memory bandwidth.
* **Mega-Buffers:** Packing multiple meshes or LOD levels into a single large vertex/index buffer pair allows drawing hundreds of distinct objects via `vertex_offset` and `first_index` without rebinding buffers.

### 2. The 3D Coordinate Transformation Pipeline
GPUs clip all geometry to a canonical **Clip Space** box (`X, Y` in `[-1, +1]`, `Z` in `[0, 1]` for SDL3/D3D/Metal conventions). Every vertex moves through three matrices in the Vertex Shader:
1. **Model Matrix (`Local Space → World Space`):** Positions, rotates, and scales an individual asset into the game world.
2. **View Matrix (`World Space → Camera/View Space`):** Translates and rotates the entire world inversely around the player's eye so the camera sits at `(0, 0, 0)` looking down the forward axis.
3. **Projection Matrix (`View Space → Clip Space`):**
   * **Perspective Projection:** Applies foreshortening (`W`-divide) so distant objects converge toward the center and appear smaller.
   * **Orthographic Projection:** Preserves parallel lines without depth scaling—ideal for 2D games, isometric views, UI, and directional sun shadow maps.

### 3. Uniform Buffers vs. Storage Buffers

| Attribute | Uniform Buffers (Push Constants) | Storage Buffers (`SDL_GPUBuffer`) |
| :--- | :--- | :--- |
| **Mental Model** | Small sticky note broadcast to all shader threads. | Large shared filing cabinet in VRAM. |
| **Capacity Limit** | Small (typically capped at **64 KB**). | Massive (hundreds of MBs to GBs). |
| **Access Speed** | Ultra-fast constant cache when all threads read the same address. | Fast VRAM access optimized for indexed/scattered lookups. |
| **Shader Permissions** | **Read-only**. | **Read and Write** (supports atomic operations). |
| **SDL3 Workflow** | Pushed directly via `SDL_PushGPUVertexUniformData` / `FragmentUniformData`. | Created as an `SDL_GPUBuffer` with storage usage flags and bound to Sets 0/2. |
| **Primary Engine Uses** | Camera matrices, frame time, material constants, single-object transforms. | Instance transform arrays, hundreds of dynamic lights, bone matrix palettes, compute particle pools. |

### 4. Skeletal Animation & GPU Vertex Skinning
* **Skeleton Hierarchy:** A tree of joints animated on the CPU each frame by interpolating keyframe transforms and multiplying against each joint's **Inverse Bind Matrix**.
* **Linear Blend Skinning:** Each vertex stores up to **4 Bone Indices** and **4 Bone Weights** (summing to `1.0`). The Vertex Shader transforms the vertex by each assigned bone matrix and blends the results, deforming high-poly character meshes in VRAM while uploading only a few kilobytes of bone matrices per frame.

---

## Part IV: Textures, Samplers & Physically Based Shading (PBR)

### 1. Decoupling Textures from Samplers
In `SDL_GPU`, raw image storage is strictly separated from sampling behavior:
* **Textures (`SDL_GPUTexture`):** Own VRAM storage, dimensions, pixel formats (`RGBA8`, `R16G16B16A16_FLOAT`, `D32_FLOAT`), array layers, and mipmap chains.
* **Samplers (`SDL_GPUSampler`):** Define lookup rules:
  * **Filtering:** `NEAREST` (crisp pixel art) vs. `LINEAR` (smooth bilinear/trilinear blending).
  * **Addressing / Wrapping:** `REPEAT` (tiling walls/floors), `MIRRORED_REPEAT`, or `CLAMP_TO_EDGE` (UI sprites and render targets).
  * **Anisotropic Filtering:** Preserves sharpness on textures viewed at steep grazing angles.
* **Engine Advantage:** A renderer only needs 3–5 global sampler objects reused across thousands of textures, and can sample the exact same texture with different samplers in the same shader.

### 2. Mipmaps & Hardware Block Compression
* **Mipmaps:** Pre-shrunk half-resolution chains (`1024 → 512 → ... → 1×1`) that prevent distant texture shimmering (aliasing) and improve texture cache locality at only a **33% memory overhead**. Generated via `SDL_GenerateMipmapsForGPUTexture()` or loaded pre-baked.
* **Block Compression (BCn / ASTC):** Standard PNG/JPG files must decompress into huge raw RGBA arrays in VRAM. Hardware formats like **BC7** (desktop) and **ASTC** (mobile) compress `4×4` pixel blocks at fixed byte ratios (4×–8× smaller) and decode directly inside GPU texture caches on the fly.

### 3. Lighting Evolution: Blinn-Phong to Physically Based Rendering (PBR)
* **Surface Normals & Dot Product:** Lighting intensity is governed by the dot product between the unit **Surface Normal (`N`)** and **Light Direction (`L`)** (`max(dot(N, L), 0.0)`).
* **Classic Blinn-Phong:** Approximates surfaces using **Ambient** (base fill), **Diffuse** (matte shading), and **Specular** (view-dependent shininess glint).
* **PBR (Metallic-Roughness Workflow):** Enforces **Energy Conservation** and **Microfacet Theory** so materials look realistic under any lighting environment:
  * **Albedo (`_SRGB`):** Unlit base color (or reflectance tint for metals).
  * **Metallic (`0.0–1.0`):** Distinguishes dielectrics (wood, stone, skin—white specular + colored diffuse) from conductors (gold, iron—tinted specular + zero diffuse).
  * **Roughness (`0.0–1.0`):** Controls microfacet scatter width from mirror-sharp (`0.0`) to chalk-matte (`1.0`).
  * **Normal Map:** Stores tangent-space `XYZ` normal perturbations in RGB channels to simulate fine geometric relief per pixel.
  * **Ambient Occlusion (AO):** Attenuates indirect ambient light inside crevices.
  * **Image-Based Lighting (IBL):** Samples pre-convolved **Environment Cubemaps** (`SDL_GPU_TEXTURETYPE_CUBE`) using roughness to select mip levels for realistic global reflections.

---

## Part V: Visibility, Culling, LOD & GPU-Driven Scaling

### 1. Depth Testing & Transparency Sorting
* **Depth Buffer (Z-Buffer):** An offscreen depth texture storing normalized `[0.0, 1.0]` distances. With `enable_depth_test = true` and `enable_depth_write = true`, opaque geometry renders in any order while closer pixels automatically overwrite farther ones.
* **The Transparency Rule:** Alpha blending (`SrcAlpha * SrcColor + (1 - SrcAlpha) * DstColor`) cannot mix colors through the depth buffer. Engines must split rendering into two phases:
  1. **Opaque Pass:** Draw all solid objects in any order (front-to-back is optimal for early-Z rejection) with **Depth Test ON** and **Depth Write ON**.
  2. **Transparent Pass:** Sort transparent objects on the CPU **farthest-to-closest**, keeping **Depth Test ON** (so walls occlude glass) but turning **Depth Write OFF** (`enable_depth_write = false`).

### 2. The Four-Stage Visibility Funnel
To avoid wasting GPU cycles on unseen geometry, modern engines filter objects in cascading stages:
1. **View Frustum Culling:** Tests object bounding spheres or AABBs against the 6 camera frustum planes, discarding 70–80% of off-screen world objects immediately.
2. **Occlusion Culling:** Discards objects inside the frustum that sit behind solid walls or terrain—using pre-baked **Potentially Visible Sets (PVS)** or GPU **Hierarchical-Z (Hi-Z)** depth pyramids.
3. **Hardware Backface Culling:** Configured in the graphics pipeline rasterizer to discard triangles winding away from the camera.
4. **Early-Z / Depth Testing:** Hardware pixel rejection before running expensive Fragment Shaders.

### 3. Level of Detail (LOD), Instancing & Indirect Drawing
* **Discrete LODs:** Swapping high-poly meshes (`LOD 0`) for simplified meshes (`LOD 1–3`) or flat **Impostor Billboards** based on screen-space coverage, optionally using dithered alpha cross-fading to eliminate visual popping.
* **Instanced Rendering:** Renders thousands of identical meshes (trees, grass, debris) in **one draw call** using `instance_count`, reading per-instance transforms via `InstanceIndex` from an instance-rate vertex buffer or Storage Buffer.
* **GPU-Driven Indirect Drawing (`SDL_DrawGPUIndexedPrimitivesIndirect`):** A Compute Shader performs frustum/Hi-Z culling and LOD selection across tens of thousands of objects entirely in VRAM, writing surviving draw arguments directly into an **Indirect Buffer** so the CPU issues a single blind draw call with zero per-object overhead.

---

## Part VI: Multi-Pass Architectures, Shadows, HDR & Anti-Aliasing

### 1. Offscreen Render Targets
Any `SDL_GPUTexture` created with `COLOR_TARGET` or `DEPTH_STENCIL_TARGET` plus `SAMPLER` flags can be drawn into during one Render Pass and read as a texture in subsequent passes (e.g., portals, security monitors, shadow maps, and post-processing).

### 2. Forward vs. Deferred Rendering

| Dimension | Forward Rendering | Deferred Rendering |
| :--- | :--- | :--- |
| **Pass Structure** | Lights are evaluated immediately during geometry drawing. | **Pass 1** writes unlit surface data into a multi-target **G-Buffer** (Albedo, Normals, Material, Depth); **Pass 2** computes lighting per screen pixel. |
| **Light Scalability** | `O(Objects × Lights)` — degrades with many dynamic lights. | `O(Objects + Lights)` — effortlessly handles hundreds of dynamic lights. |
| **Memory Bandwidth** | Low. | High (writes/reads multiple full-screen G-Buffer attachments). |
| **Transparency & MSAA** | Native and straightforward. | Requires a separate Forward pass for transparency; relies on TAA/FXAA instead of MSAA. |

*(Hybrid **Forward+ / Tiled Forward** uses a Compute Shader to bin lights into screen-space tiles, preserving Forward rendering's low bandwidth and MSAA support while scaling to high light counts.)*

### 3. Real-Time Shadow Mapping
* **Two-Pass Depth Comparison:** Pass 1 renders shadow casters from the light's viewpoint into a depth-only **Shadow Map** (`num_color_targets = 0`). Pass 2 transforms each shaded pixel into light space and compares its depth against the Shadow Map using a hardware **Comparison Sampler**.
* **Artifact Mitigation:**
  * **Slope-Scaled Depth Bias:** Eliminates striped **Shadow Acne** caused by discrete texel quantization.
  * **Percentage-Closer Filtering (PCF):** Samples a multi-tap neighborhood in the Shadow Map to produce soft shadow penumbras.
  * **Cascaded Shadow Maps (CSM):** Splits the camera frustum into near, mid, and far depth slices so close shadows stay crisp while distant terrain remains shadowed.

### 4. HDR Rendering, Tone Mapping & Linear Color Workflow
* **Linear Input (`_SRGB` textures):** Sampling `_SRGB` textures automatically converts gamma-encoded artist assets into Linear light space for accurate shading math.
* **Floating-Point HDR Target (`R16G16B16A16_FLOAT`):** Prevents bright lights, specular highlights, and emissive surfaces from clipping at `1.0`, enabling physically accurate **Bloom** extraction.
* **Tone Mapping (`ACES Filmic`):** Smoothly compresses unbounded HDR luminance values back into `[0.0, 1.0]` display range while preserving shadow contrast and highlight roll-off, outputting into an `_SRGB` swapchain texture for automatic monitor gamma encoding.

### 5. Anti-Aliasing Strategies
* **MSAA (Hardware Multisampling):** Evaluates coverage at 2×/4×/8× sub-pixel locations while running the Fragment Shader once per pixel, resolving via `resolve_texture` in `SDL_GPUColorTargetInfo`. Sharp geometry, high bandwidth.
* **FXAA (Fast Approximate):** Ultra-fast 2D post-process edge blur. Zero setup complexity, slight texture softening.
* **TAA (Temporal Anti-Aliasing):** Jitters the projection matrix by sub-pixel offsets every frame and accumulates samples over time using **Motion Vectors** and history render targets. Eliminates both edge jaggies and specular shimmering at low cost.

---

## Part VII: Compute Shaders & GPU Profiling/Debugging

### 1. General-Purpose Compute (`SDL_GPUComputePass`)
Compute Shaders bypass vertex assembly and rasterization to run arbitrary parallel algorithms across 3D grids of **Workgroups** and **Threads** via `SDL_DispatchGPUCompute()`:
* **Engine Applications:** GPU particle simulations (updating 500,000+ particles in Storage Buffers), frustum/Hi-Z occlusion culling, tiled light culling, fast separable Gaussian blurs for bloom, and procedural terrain/noise generation.
* **Zero-Copy Pipeline Handoff:** Storage Buffers written during a Compute Pass can be bound directly as Vertex or Indirect Buffers in the immediately following Render Pass.

### 2. GPU Debugging & Profiling Workflow
Because GPU execution is asynchronous, frame-capture tools are indispensable when developing a custom renderer:
* **Validation Layers (`debug_mode = true`):** Always enable during development in `SDL_CreateGPUDevice()` to catch invalid resource states, mismatched pipeline formats, and missing bindings immediately.
* **Frame Capture Tools:**
  * **RenderDoc (Windows/Linux):** Inspects Vulkan/D3D12 draw calls, pre/post-vertex meshes, bound textures, and step-by-step pixel shader execution.
  * **PIX on Windows (D3D12) & Xcode Metal Frame Debugger (macOS/iOS):** Provide deep hardware counter timelines and per-pass GPU microsecond timings.
* **Instrumentation in SDL3 GPU:** Wrap logical passes in **`SDL_PushGPUDebugGroup()` / `SDL_PopGPUDebugGroup()`** and label buffers/textures via **`SDL_SetGPUBufferName()` / `SDL_SetGPUTextureName()`** so frame captures display clean, human-readable hierarchy folders.

---

## Part VIII: End-to-End Engine Frame Blueprint

When architecting your custom engine's `Renderer::RenderFrame()`, organize every frame into this deterministic, multi-pass flow:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ 1. CPU PREP & STREAMING                                                     │
│    • Update camera View/Projection matrices & skeletal bone palettes        │
│    • Perform coarse CPU frustum culling (or upload scene bounds to GPU)      │
│    • Map Transfer Buffers (cycle = true) for dynamic UI / particle updates  │
└──────────────────────────────────────┬──────────────────────────────────────┘
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 2. ACQUIRE COMMAND BUFFER & COPY PASS                                       │
│    • Acquire SDL_GPUCommandBuffer                                           │
│    • Begin Copy Pass -> Upload staged dynamic buffers/textures -> End Pass  │
└──────────────────────────────────────┬──────────────────────────────────────┘
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 3. COMPUTE SIMULATION & GPU CULLING PASS                                    │
│    • Step GPU particle physics & cloth simulations in Storage Buffers       │
│    • Run Frustum + Hi-Z Occlusion Culling -> Write Indirect Draw Buffers    │
└──────────────────────────────────────┬──────────────────────────────────────┘
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 4. SHADOW MAPPING PASS(ES) (Depth-Only Offscreen Targets)                   │
│    • Render opaque + skinned casters from directional/spot light views      │
│    • Write Cascaded Shadow Maps (num_color_targets = 0)                     │
└──────────────────────────────────────┬──────────────────────────────────────┘
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 5. MAIN HDR SCENE PASS (R16G16B16A16_FLOAT + Depth Target)                  │
│    • Clear HDR color & depth targets                                        │
│    • Draw Opaque Geometry (Forward+ or Deferred G-Buffer + Lighting)        │
│      with PBR materials, Normal Maps, Shadow Map PCF & IBL Cubemaps         │
│    • Draw Skybox at maximum depth                                           │
│    • Draw Back-to-Front Sorted Transparent Geometry (Depth Write OFF)       │
└──────────────────────────────────────┬──────────────────────────────────────┘
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 6. POST-PROCESSING & SWAPCHAIN PRESENT                                      │
│    • Run TAA resolve (using Motion Vectors + History Target)                │
│    • Extract & blur HDR highlights (> 1.0) for Bloom                        │
│    • Acquire Window Swapchain Texture (handles automatic frame pacing)      │
│    • Final Composite Pass -> ACES Tone Mapping + Bloom -> _SRGB Swapchain   │
│    • Render crisp 2D HUD / UI overlay directly onto Swapchain               │
│    • Submit Command Buffer to GPU Queue                                     │
└─────────────────────────────────────────────────────────────────────────────┘
```
