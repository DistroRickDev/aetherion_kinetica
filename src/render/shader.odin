package render

import sdl "vendor:sdl3"

ShaderObject :: struct {
	shader_data:     []u8,
	shader_metadata: struct {
		num_of_samplers:     u32,
		storage_textures:    u32,
		storage_buffers:     u32,
		num_uniform_buffers: u32,
	},
}

load_shader :: proc(
	device: ^sdl.GPUDevice,
	code: []u8,
	stage: sdl.GPUShaderStage,
	num_uniform_buffers: u32,
) -> ^sdl.GPUShader {
	return sdl.CreateGPUShader(
		device,
		{
			code_size = len(code),
			code = raw_data(code),
			entrypoint = "main",
			format = {.SPIRV},
			stage = stage,
			num_uniform_buffers = num_uniform_buffers,
		},
	)
}
