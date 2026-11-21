package render

import sdl "../ffi/sdl3"
import "core:fmt"
import "core:math/linalg"
import "core:mem"

WindowSettings :: struct {
	window_size:  [2]i32,
	window_name:  cstring,
	window_flags: sdl.WindowFlags,
}

BufferSlot :: struct {
	gpu_buffer:  ^sdl.GPUBuffer,
	buffer_size: u32,
	usage:       sdl.GPUBufferUsageFlags,
	in_use:      bool,
}

@(private)
RenderState :: struct {
	main_window:    ^sdl.Window,
	device:         ^sdl.GPUDevice,
	pipeline:       ^sdl.GPUGraphicsPipeline,
	vertex_buffers: [dynamic]BufferSlot,
	vertex_shader_obj: Shader_Object,
	fragment_shader_obj: Shader_Object
}

@(private)
render_state := RenderState{}

get_window :: proc() -> ^sdl.Window {
	assert(render_state.main_window != nil)
	return render_state.main_window
}

get_device :: proc() -> ^sdl.GPUDevice {
	assert(render_state.device != nil)
	return render_state.device
}

init :: proc(settings: WindowSettings) {
	//sdl.SetHint(sdl.HINT_VIDEO_DRIVER, "x11")
	ok := sdl.Init({.VIDEO}); assert(ok)

	render_state.main_window = sdl.CreateWindow(
		settings.window_name,
		settings.window_size.x,
		settings.window_size.y,
		settings.window_flags,
	); assert(render_state.main_window != nil)

	render_state.device = sdl.CreateGPUDevice(
		{.SPIRV},
		true,
		nil,
	); assert(render_state.device != nil)
	ok = sdl.ClaimWindowForGPUDevice(render_state.device, render_state.main_window); assert(ok)
}

destroy_render_resources :: proc() {
	ok := sdl.WaitForGPUIdle(render_state.device); assert(ok)
	clean_vertex_buffers()
	clean_shader_object(&render_state.vertex_shader_obj)
	clean_shader_object(&render_state.fragment_shader_obj)
	sdl.ReleaseGPUGraphicsPipeline(render_state.device, render_state.pipeline)
	sdl.DestroyGPUDevice(render_state.device)
	sdl.DestroyWindow(render_state.main_window)
}

@(private)
clean_vertex_buffers :: proc() {
	for buffer in render_state.vertex_buffers {
		if buffer.gpu_buffer != nil {
			sdl.ReleaseGPUBuffer(render_state.device, buffer.gpu_buffer)
		}
	}
	delete(render_state.vertex_buffers)
}

@(private)
transfer_single_buffer_to_gpu :: proc(vertices: []VertexData) {
	data_size := render_state.vertex_buffers[0].buffer_size
	data := raw_data(vertices)

	transfer_buf := sdl.CreateGPUTransferBuffer(
		render_state.device,
		{usage = .UPLOAD, size = u32(data_size)},
	)
	defer (sdl.ReleaseGPUTransferBuffer(render_state.device, transfer_buf))

	transfer_mem := transmute([^]byte)sdl.MapGPUTransferBuffer(
		render_state.device,
		transfer_buf,
		false,
	)
	mem.copy(transfer_mem, data, int(data_size))
	sdl.UnmapGPUTransferBuffer(render_state.device, transfer_buf)

	copy_cmd_buffer := sdl.AcquireGPUCommandBuffer(render_state.device)
	copy_pass := sdl.BeginGPUCopyPass(copy_cmd_buffer)

	sdl.UploadToGPUBuffer(
		copy_pass,
		{transfer_buffer = transfer_buf},
		{buffer = render_state.vertex_buffers[0].gpu_buffer, size = data_size},
		false,
	)

	sdl.EndGPUCopyPass(copy_pass)
	ok := sdl.SubmitGPUCommandBuffer(copy_cmd_buffer); assert(ok)
}

@(private)
create_gpu_graphics_pipeline :: proc(
	vertex_shader: ^sdl.GPUShader,
	fragment_shader: ^sdl.GPUShader,
) {

	temp_vertex_attr := make(
		[]sdl.GPUVertexAttribute,
		len(render_state.vertex_shader_obj.shader_metadata.inputs.resources),
	)
	defer delete(temp_vertex_attr)
	current_offset: u32 = 0
	for &input, idx in render_state.vertex_shader_obj.shader_metadata.inputs.resources {
		temp_vertex_attr[idx] = {
			location = input.location,
			format   = input.format,
			offset   = current_offset,
		}
		current_offset += format_to_size(input.format)
	}
	vertex_attr := temp_vertex_attr[:]

	render_state.pipeline = sdl.CreateGPUGraphicsPipeline(
		render_state.device,
		{
			vertex_shader = vertex_shader,
			fragment_shader = fragment_shader,
			primitive_type = .TRIANGLELIST,
			vertex_input_state = {
				num_vertex_buffers = 1,
				vertex_buffer_descriptions = &(sdl.GPUVertexBufferDescription {
						slot = 0,
						pitch = size_of(VertexData),
					}),
				num_vertex_attributes = u32(len(vertex_attr)),
				vertex_attributes = raw_data(vertex_attr),
			},
			target_info = {
				num_color_targets = 1,
				color_target_descriptions = &(sdl.GPUColorTargetDescription {
						format = sdl.GetGPUSwapchainTextureFormat(
							render_state.device,
							render_state.main_window,
						),
					}),
			},
		},
	)
}

@(private)
release_shaders :: proc(vertex_shader: ^sdl.GPUShader, fragment_shader: ^sdl.GPUShader) {
	sdl.ReleaseGPUShader(render_state.device, vertex_shader)
	sdl.ReleaseGPUShader(render_state.device, fragment_shader)
}

@(private)
ROTATION_SPEED := linalg.to_radians(f32(90))
@(private)
rotation := f32(0)
@(private)
proj_mat: linalg.Matrix4f32

set_render_primitive :: proc(
	vertices: []VertexData,
	vertex_shader_byte_buffer: []u8,
	fragment_shader_byte_buffer: []u8,
) {
	/* 1.- create_shaders */
	render_state.vertex_shader_obj = get_shader_object_from_data(vertex_shader_byte_buffer)

	vertex_shader := load_shader(render_state.device, render_state.vertex_shader_obj, .VERTEX)

	render_state.fragment_shader_obj = get_shader_object_from_data(fragment_shader_byte_buffer)

	fragment_shader := load_shader(render_state.device, render_state.fragment_shader_obj, .FRAGMENT)

	/* 2.- create uniform buffer */
	vertices_byte_size := get_vertex_buffer_size(vertices)

	append(
		&render_state.vertex_buffers,
		{
			gpu_buffer = sdl.CreateGPUBuffer(
				render_state.device,
				{usage = {.VERTEX}, size = u32(vertices_byte_size)},
			),
			buffer_size = vertices_byte_size,
			usage = {.VERTEX},
			in_use = true,
		},
	)

	/* 3.- Transfer buffer to GPU
		   -> Create transfer buffer
		   -> Copy vertex data
		   -> Transfer to GPU
	*/
	transfer_single_buffer_to_gpu(vertices)

	/* 4.- Create GPU graphics pipeline */
	create_gpu_graphics_pipeline(
		vertex_shader,
		fragment_shader,
	)

	/* 5.- Release shaders */
	release_shaders(vertex_shader, fragment_shader)

	win_size: [2]i32
	ok := sdl.GetWindowSize(render_state.main_window, &win_size.x, &win_size.y); assert(ok)

	proj_mat = linalg.matrix4_perspective_f32(
		linalg.to_radians(f32(70)),
		f32(win_size.x) / f32(win_size.y),
		0.0001,
		1000,
	)
}

render_frame :: proc(delta_time: f32) {
	cmd_buf := sdl.AcquireGPUCommandBuffer(render_state.device)
	swapchain_tex: ^sdl.GPUTexture

	ok := sdl.WaitAndAcquireGPUSwapchainTexture(
		cmd_buf,
		render_state.main_window,
		&swapchain_tex,
		nil,
		nil,
	); assert(ok)

	rotation += ROTATION_SPEED * delta_time
	model_mat :=
		linalg.matrix4_translate_f32({0, 0, -5}) * linalg.matrix4_rotate_f32(rotation, {0, 1, 0})

	ubo := UBO {
		mvp = proj_mat * model_mat,
	}

	if swapchain_tex != nil {
		color_target := sdl.GPUColorTargetInfo {
			texture     = swapchain_tex,
			load_op     = .CLEAR,
			clear_color = {0, 0.2, 0.4, 1},
			store_op    = .STORE,
		}
		render_pass := sdl.BeginGPURenderPass(cmd_buf, &color_target, 1, nil)
		sdl.BindGPUGraphicsPipeline(render_pass, render_state.pipeline)
		sdl.BindGPUVertexBuffers(
			render_pass,
			0,
			&(sdl.GPUBufferBinding{buffer = render_state.vertex_buffers[0].gpu_buffer}),
			1,
		)
		//sdl.BindGPUIndexBuffer(render_pass, {buffer = index_buf}, ._16BIT)
		sdl.PushGPUVertexUniformData(cmd_buf, 0, &ubo, size_of(ubo))
		sdl.DrawGPUPrimitives(render_pass, 3, 1, 0, 0)
		//sdl.DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0)
		sdl.EndGPURenderPass(render_pass)
	}

	ok = sdl.SubmitGPUCommandBuffer(cmd_buf); assert(ok)
}
