package render

import sdl "../ffi/sdl3"
import spvc "../ffi/spirv_cross"
import "base:runtime"
import "core:c"
import "core:log"

ShaderObject :: struct {
	shader_data:     []u8,
	shader_metadata: struct {
		num_of_samplers:     u32,
		storage_textures:    u32,
		storage_buffers:     u32,
		num_uniform_buffers: u32,
	},
}

as_u32_buffer :: proc(code: []u8) -> []u32 {
	raw := transmute(runtime.Raw_Slice)code
	raw.len = len(code) / 4
	// raw.cap = len(bytes) / 4 // optional if capacity matters
	return transmute([]u32)raw
}

get_shader_object_from_data :: proc(code: []u8) -> ShaderObject {
	ctx: spvc.Context
	err := spvc.context_create(&ctx); assert(err == .Success)
	defer spvc.context_destroy(ctx)

	ir: spvc.Parsed_IR
	data := as_u32_buffer(code)
	err = spvc.context_parse_spirv(ctx, raw_data(data), len(data), &ir); assert(err == .Success)

	compiler: spvc.Compiler
	err = spvc.context_create_compiler(ctx, .GLSL, ir, .Copy, &compiler); assert(err == .Success)

	resources: spvc.Resources
	err = spvc.compiler_create_shader_resources(compiler, &resources); assert(err == .Success)

	uniform_buffers: spvc.Reflected_Resource
	num_uniform_buffers: c.size_t
	err = spvc.resources_get_resource_list_for_type(
		resources,
		.UNIFORM_BUFFER,
		&uniform_buffers,
		&num_uniform_buffers,
	); assert(err == .Success)

	return {shader_data = code, shader_metadata = {num_uniform_buffers = u32(num_uniform_buffers)}}
}

load_shader :: proc(
	device: ^sdl.GPUDevice,
	shader_object: ShaderObject,
	stage: sdl.GPUShaderStage,
) -> ^sdl.GPUShader {
	return sdl.CreateGPUShader(
		device,
		{
			code_size = len(shader_object.shader_data),
			code = raw_data(shader_object.shader_data),
			entrypoint = "main",
			format = {.SPIRV},
			stage = stage,
			num_uniform_buffers = shader_object.shader_metadata.num_uniform_buffers,
			num_samplers = shader_object.shader_metadata.num_of_samplers,
			num_storage_buffers = shader_object.shader_metadata.storage_buffers,
			num_storage_textures = shader_object.shader_metadata.storage_textures,
		},
	)
}
