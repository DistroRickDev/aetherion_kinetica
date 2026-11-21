
# Dynamic SPIRV-Cross Shader Reflection & Pipeline Architecture for Odin + SDL_GPU

This document compiles the complete architectural blueprint for integrating SPIRV-Cross into a custom Odin 3D game engine targeting SDL_GPU, enabling zero-boilerplate FFI bindings, dynamic SPIR-V reflection, automatic vertex/instance layout extraction, and on-demand graphics pipeline caching.

## 1. Odin FFI & SPIRV-Cross Integration

Odin speaks the C ABI natively without a heavy runtime, making it ideal for binding directly to external C libraries. Because SPIRV-Cross is written in C++11, you must bind to its official C89 wrapper (spirv_cross_c.h) compiled as a shared library (spirv-cross-c-shared) so the C++ runtime remains encapsulated inside the dynamic library.

### Key FFI Mechanics in Odin

- Library Linking: Use foreign import spvc "spirv-cross-c-shared.lib" (or .dll, .so, .dylib via when ODIN_OS == ... blocks).
- Calling Conventions & Prefixes: Wrap declarations inside a foreign spvc { ... } block annotated with @(default_calling_convention="c") and @(link_prefix="spvc_").
- Opaque Handles: Represent SPIRV-Cross internal pointers (spvc_context, spvc_parsed_ir, spvc_compiler, spvc_resources, spvc_type) as distinct rawptr types for compile-time type safety.
- Memory Management: SPIRV-Cross allocates all reflection lists, strings, and IR nodes inside an arena owned by spvc_context. Calling spvc_context_destroy(ctx) once at the end of reflection frees all associated memory in bulk without per-item deallocations.

## 2. Offline Baking vs. Dynamic Runtime Reflection

While offline reflection during asset baking eliminates runtime parsing overhead and removes spirv-cross from release binaries, dynamic runtime reflection allows your engine to load arbitrary .spv shaders on the fly, support instant hot-reloading, and adapt pipeline layouts automatically.

Optimizing Runtime Reflection Performance

To keep dynamic reflection fast enough for seamless loading and hot-reloading:
1. Use SPVC_BACKEND_NONE: Pass .None to spvc_context_create_compiler. This initializes only the SPIR-V reflection engine and skips all GLSL/HLSL/MSL AST generation and string-building overhead.
2. Read Raw C Structs Directly: Avoid the JSON reflection backend. Call spvc_resources_get_resource_list_for_type to iterate directly over spvc_reflected_resource arrays in memory.
3. Cache Reflected Metadata Per Shader: Store the extracted metadata struct alongside the SDL_GPUShader handle in a cache keyed by shader path or bytecode hash. Run reflection once on load (or hot-reload), destroy spvc_context immediately, and reuse the cached metadata across all materials and pipelines.
4. Optional Cross-Platform Transpilation: Because the spvc_parsed_ir is already in memory during reflection, you can check SDL_GetGPUShaderFormats(device) and instantiate a second spvc_compiler with .HLSL or .MSL on the exact same IR if running on D3D12 or Metal.

## 3. Mapping SPIR-V Reflection to SDL_GPU Rules

SDL_GPU enforces strict descriptor set conventions and requires exact resource counts upfront when creating shader objects (SDL_CreateGPUShader).

### A. Stage Resource Counts & Set Validation

Query spvc_resources for each resource category to populate SDL_GPUShaderCreateInfo:

SDL_GPU Field	SPIRV-Cross Resource Type	Validation Rule (Descriptor Set)
num_uniform_buffers	SPVC_RESOURCE_TYPE_UNIFORM_BUFFER	Vertex: Set 1 / Fragment: Set 3
num_samplers	SPVC_RESOURCE_TYPE_SAMPLED_IMAGE	Vertex: Set 0 / Fragment: Set 2
num_storage_textures	SPVC_RESOURCE_TYPE_STORAGE_IMAGE	Vertex: Set 0 / Fragment: Set 2
num_storage_buffers	SPVC_RESOURCE_TYPE_STORAGE_BUFFER	Vertex: Set 0 / Fragment: Set 2

• Set Enforcement: Query SPVC_DECORATION_DESCRIPTOR_SET via spvc_compiler_get_decoration for every resource. Log an engine error immediately if a shader violates SDL_GPU's stage-to-set mapping.
• Array Handling: Check spvc_type_get_num_array_dimensions on res.type_id. If an array is declared (e.g., uniform sampler2D textures[4]), add spvc_type_get_array_dimension(type, 0) to the resource count rather than counting it as 1.

| Stage | Set | Holds (each set's bindings numbered from 0 in this order) |
| :--- | :---: | :--- |
| vertex | 0 | samplers, then storage textures, then storage buffers (read-only) |
| vertex | 1 | uniform blocks |
| fragment | 2 | samplers, then storage textures, then storage buffers (read-only) |
| fragment | 3 | uniform blocks |
| compute | 0 | samplers, then read-only storage textures, then read-only storage buffers |
| compute | 1 | read-write storage textures, then read-write storage buffers |
| compute | 2 | uniform blocks |

### B. Raw Struct Uploads for Uniform Buffers (No Member Reflection)

Instead of reflecting individual member names and offsets inside UBOs, treat uniform blocks as opaque byte blobs bound to numeric slots (0..3):
• Extracted Data: For each UNIFORM_BUFFER, record only its SPVC_DECORATION_BINDING slot and its total std140 padded byte size via spvc_compiler_get_declared_struct_size on res.base_type_id.
• Storage: Store a fixed-size array per shader stage (ubo_sizes: [4]u32, initialized to 0).
• Debug Validation: When calling SDL_PushGPUVertexUniformData or SDL_PushGPUFragmentUniformData with an Odin struct of type T, assert in debug builds that ubo_sizes[slot] > 0 and size_of(T) == ubo_sizes[slot]. This catches struct layout mismatches without string lookups.

### C. Vertex Inputs via Fixed Location Ranges

To distinguish per-vertex mesh attributes from per-instance data automatically without naming conventions or manual vertex descriptors, enforce fixed location ranges in your shader specification:
• Locations 0–7 → Buffer Slot 0 (SDL_GPU_VERTEXINPUTRATE_VERTEX)
• Locations 8–15 → Buffer Slot 1 (SDL_GPU_VERTEXINPUTRATE_INSTANCE)

Extraction Algorithm for Vertex STAGE_INPUT

1. Filter Built-ins: Ignore SPVC_RESOURCE_TYPE_BUILTIN_INPUT (such as gl_VertexIndex and gl_InstanceIndex).
2. Partition by Location: Read SPVC_DECORATION_LOCATION. Route location < 8 to Slot 0 and location >= 8 to Slot 1.
3. Sort Ascending: Sort each slot's attributes by location ascending so byte offsets match deterministic interleaved struct layouts regardless of GLSL declaration order.
4. Map Formats & Expand Matrices: Inspect basetype, vector_size, and columns:
	• Standard vectors (columns == 1) map directly to SDL_GPUVertexElementFormat (e.g., Float + 3 → FLOAT3, Float + 4 → FLOAT4).
	• Matrix inputs (columns > 1, such as an instance mat4 at location = 8) emit columns consecutive vector attributes (locations 8, 9, 10, 11 as FLOAT4), advancing the byte offset per column.
5. Compute Strides & Active Buffers: Accumulate byte offsets independently for Slot 0 (vertex_pitch) and Slot 1 (instance_pitch). Only populate SDL_GPUVertexBufferDescription entries for slots that contain at least one attribute (if no attributes exist at location >= 8, set num_vertex_buffers = 1).

### D. Stage Interface & Fragment Outputs

• Vertex-to-Fragment Validation: Record STAGE_OUTPUT locations and types on the vertex shader, and STAGE_INPUT locations and types on the fragment shader. Validate that every fragment input location exists in the vertex output with a matching base type and vector width before creating a pipeline.
• Fragment Render Targets: Inspect STAGE_OUTPUT on the fragment shader to record the number of active color targets (locations 0..N) and their base types (Float, Int, Uint) to verify compatibility with the bound render pass textures.

## 4. Compact Reflected Shader Struct Layout

Using fixed location ranges and raw UBO sizes allows the entire reflected footprint of any shader to live in a flat, cache-friendly Odin struct without heap-allocated strings:

``` odin
odin
Reflected_Resource_Info :: struct {
	id:           u32,
	base_type_id: u32,
	type_id:      u32,
	name:         string, // Cloned from spvc.Reflected_Resource.name so it outlives spvc_context
	set:          u32,    // Optional: queried via compiler_get_decoration(..., .DescriptorSet)
	binding:      u32,    // Optional: queried via compiler_get_decoration(..., .Binding)
}

Shader_Metadata :: struct {
	stage_inputs:            []Reflected_Resource_Info,
	stage_outputs:           []Reflected_Resource_Info,
	uniform_buffers:         []Reflected_Resource_Info,
	storage_buffers:         []Reflected_Resource_Info,
	sampled_images:          []Reflected_Resource_Info, // Combined sampler2D, etc.
	separate_images:         []Reflected_Resource_Info, // texture2D, etc.
	separate_samplers:       []Reflected_Resource_Info, // sampler, samplerShadow
	storage_images:          []Reflected_Resource_Info, // image2D (read/write)
	push_constants:          []Reflected_Resource_Info,
	subpass_inputs:          []Reflected_Resource_Info,
	acceleration_structures: []Reflected_Resource_Info,
}

Shader_Object :: struct {
	shader_data:     []u8,
	shader_metadata: Shader_Metadata,
}
```

Use code with caution.

## 5. Dynamic Graphics Pipeline Creation & Caching

Because SPIR-V only describes programmable stages, assembling a SDL_GPUGraphicsPipeline requires combining reflected shader metadata with fixed-function states and active render pass attachment formats.

Splitting Pipeline State by Source

```bash

┌───────────────────────────┐   ┌───────────────────────────┐   ┌───────────────────────────┐
│     Shader Reflection     │   │   Material / Draw State   │   │    Active Render Pass     │
├───────────────────────────┤   ├───────────────────────────┤   ├───────────────────────────┤
│ • Vertex & Frag Handles   │   │ • Primitive Topology      │   │ • Color Target Formats    │
│ • Vertex Buffer Strides   │   │ • Cull & Fill Modes       │   │ • Color Target Count      │
│ • Vertex Attributes       │   │ • Depth Test / Write Ops  │   │ • Depth/Stencil Format    │
│ • Stage I/O Validation    │   │ • Blend Mode Presets      │   │ • MSAA Sample Count       │
└─────────────┬─────────────┘   └─────────────┬─────────────┘   └─────────────┬─────────────┘
              │                               │                               │
              └───────────────────────────────┼───────────────────────────────┘
                                              ▼
                              ┌───────────────────────────────┐
                              │     Pipeline_Key (Hashed)     │
                              └───────────────┬───────────────┘
                                              │
                              ┌───────────────┴───────────────┐
                              ▼                               ▼
                       [ Cache Hit ]                   [ Cache Miss ]
                    Bind Cached PSO            Assemble CreateInfo, Compile,
                                                 Insert into Map, and Bind

```

1. From Shader Reflection (Automatic): Supplies vertex_shader, fragment_shader, and the complete SDL_GPUVertexInputState.
2. From Material / Draw State (Explicit): Supplies primitive_type, rasterizer_state (cull mode, fill mode, front face), depth_stencil_state (depth test, depth write, compare op), and blend presets (Opaque, Alpha_Blend, Additive).
3. From Active Render Pass (Contextual): Captures the bound attachment formats (Render_Pass_Signature) when SDL_BeginGPURenderPass is called. Tying target formats to the active pass rather than the material allows the same shader to be reused seamlessly across shadow passes (depth-only), G-Buffer passes (MRT float targets), and swapchain forward passes.

## On-Demand PSO Cache & Hot-Reloading

• Pipeline Key: Pack vertex_shader_id, fragment_shader_id, fixed-function state enums, and Render_Pass_Signature into a single plain-data Pipeline_Key struct with no pointers.
• Lookup: Hash the struct bytes (hash.fnv64a(mem.ptr_to_bytes(&key))) and look up the ^SDL_GPUGraphicsPipeline in a map[u64]^SDL_GPUGraphicsPipeline.
• Lazy Compilation: On a cache miss, validate stage I/O and color target counts against Reflected_Shader_Info, construct SDL_GPUGraphicsPipelineCreateInfo, call SDL_CreateGPUGraphicsPipeline, store the result in the map, and bind it.
• Seamless Hot-Reloading: When a .spv file changes on disk, re-run reflection, create a new SDL_GPUShader with an incremented monotonic shader_id, and swap the handle. Any modified vertex attributes (e.g., adding a new instance attribute at location = 12) automatically trigger a PSO cache miss on the next draw call and build an updated pipeline with zero engine recompilation.
form pushes
