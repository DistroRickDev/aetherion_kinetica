package main

import "base:runtime"
import "core:math/linalg"
import "core:mem"
import "logger"
import "render"
import sdl "vendor:sdl3"

EXAMPLE_FRAG_SHADER_PATH :: "/home/distrorickdev/projects/aetherion_kinetica/examples/shaders/glsl/shader.spv.frag"
EXAMPLE_VERT_SHADER_PATH :: "/home/distrorickdev/projects/aetherion_kinetica/examples/shaders/glsl/shader.spv.vert"

FRAG_SHADER_DATA := #load(EXAMPLE_FRAG_SHADER_PATH)
VERT_SHADER_DATA := #load(EXAMPLE_VERT_SHADER_PATH)

main :: proc() {
	logger.init_logger()

	//sdl.SetHint(sdl.HINT_VIDEO_DRIVER, "x11")
	ok := sdl.Init({.VIDEO}); assert(ok)

	render.init({ window_name= "Hello SDL3", window_size =  {1280, 780}, window_flags = {}})
	defer(render.destroy_gpu_resources())

	gpu := render.get_device()
	window := render.get_window()

	ok = sdl.ClaimWindowForGPUDevice(gpu, window); assert(ok)

	vert_shader := render.load_shader(gpu, VERT_SHADER_DATA, .VERTEX, 1)
	frag_shader := render.load_shader(gpu, FRAG_SHADER_DATA, .FRAGMENT, 0)

	Vec3 :: [3]f32

	VertexData :: struct {
		pos:   Vec3,
		color: sdl.FColor,
	}

	//vertices := []VertexData {
	//	{pos = {-0.5, -0.5, 0}, color = {1, 0, 0, 1}},
	//	{pos = {0, 0.5, 0}, color = {0, 1, 1, 1}},
	//	{pos = {0.5, -0.5, 0}, color = {1, 0, 1, 1}},
	//}

	vertices := []VertexData {
		{pos = {-0.5, 0.5, 0}, color = {1, 0, 0, 1}}, // TL
		{pos = {0.5, 0.5, 0}, color = {0, 1, 1, 1}}, //TR
		{pos = {-0.5, -0.5, 0}, color = {1, 0, 1, 1}}, // BL
		{pos = {0.5, -0.5, 0}, color = {1, 0, 1, 1}}, // BR
	}
	vertices_byte_size := len(vertices) * size_of(vertices[0])

	indices := []u16 {
		0,1,2, // TL -> TR -> BL
		2,1,3  // BL -> TR -> BR
	}
	indices_byte_size :=  len(indices) * size_of(indices[0])

	vertex_buf := sdl.CreateGPUBuffer(gpu, {usage = {.VERTEX}, size = u32(vertices_byte_size)})
	defer(sdl.ReleaseGPUBuffer(gpu, vertex_buf))
	index_buf := sdl.CreateGPUBuffer(gpu, {usage={.INDEX}, size=u32(indices_byte_size)})
	defer(sdl.ReleaseGPUBuffer(gpu, index_buf))

	transfer_buf := sdl.CreateGPUTransferBuffer(
		gpu,
		{usage = .UPLOAD, size = u32(vertices_byte_size + indices_byte_size)},
	)

	transfer_mem := transmute([^]byte)sdl.MapGPUTransferBuffer(gpu, transfer_buf, false)
	mem.copy(transfer_mem, raw_data(vertices), vertices_byte_size)
	mem.copy(transfer_mem[vertices_byte_size:], raw_data(indices), indices_byte_size)
	sdl.UnmapGPUTransferBuffer(gpu, transfer_buf)

	copy_cmd_buffer := sdl.AcquireGPUCommandBuffer(gpu)
	copy_pass := sdl.BeginGPUCopyPass(copy_cmd_buffer)

	sdl.UploadToGPUBuffer(
		copy_pass,
		{transfer_buffer = transfer_buf},
		{buffer = vertex_buf, size = u32(vertices_byte_size)},
		false,
	)

	sdl.UploadToGPUBuffer(
		copy_pass,
		{transfer_buffer = transfer_buf, offset=u32(vertices_byte_size)},
		{buffer = index_buf, size = u32(indices_byte_size)},
		false,
	)

	sdl.EndGPUCopyPass(copy_pass)
	ok = sdl.SubmitGPUCommandBuffer(copy_cmd_buffer); assert(ok)
	sdl.ReleaseGPUTransferBuffer(gpu, transfer_buf)

	vertex_attr := []sdl.GPUVertexAttribute {
		{location = 0, format = .FLOAT3, offset = u32(offset_of(VertexData, pos))},
		{location = 1, format = .FLOAT4, offset = u32(offset_of(VertexData, color))},
	}

	pipeline := sdl.CreateGPUGraphicsPipeline(
		gpu,
		{
			vertex_shader = vert_shader,
			fragment_shader = frag_shader,
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
						format = sdl.GetGPUSwapchainTextureFormat(gpu, window),
					}),
			},
		},
	)
	defer(sdl.ReleaseGPUGraphicsPipeline(gpu, pipeline))
	sdl.ReleaseGPUShader(gpu, vert_shader)
	sdl.ReleaseGPUShader(gpu, frag_shader)

	win_size: [2]i32
	ok = sdl.GetWindowSize(window, &win_size.x, &win_size.y); assert(ok)

	ROTATION_SPEED := linalg.to_radians(f32(90))
	rotation := f32(0)

	proj_mat := linalg.matrix4_perspective_f32(
		linalg.to_radians(f32(70)),
		f32(win_size.x) / f32(win_size.y),
		0.0001,
		1000,
	)

	UBO :: struct {
		// model -> view -> projection
		mvp: matrix[4, 4]f32,
	}

	last_ticks := sdl.GetTicks()

	main_loop: for {
		new_ticks := sdl.GetTicks()
		delta_time := f32(new_ticks - last_ticks) / 1000
		last_ticks = new_ticks

		// process events
		ev: sdl.Event
		for sdl.PollEvent(&ev) {
			#partial switch ev.type {
			case .QUIT:
				break main_loop
			case .KEY_DOWN:
				if ev.key.scancode == .ESCAPE do break main_loop
			}
		}

		// update game state

		// render
		cmd_buf := sdl.AcquireGPUCommandBuffer(gpu)
		swapchain_tex: ^sdl.GPUTexture
		ok = sdl.WaitAndAcquireGPUSwapchainTexture(
			cmd_buf,
			window,
			&swapchain_tex,
			nil,
			nil,
		); assert(ok)

		rotation += ROTATION_SPEED * delta_time
		model_mat :=
			linalg.matrix4_translate_f32({0, 0, -5}) *
			linalg.matrix4_rotate_f32(rotation, {0, 1, 0})

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
			sdl.BindGPUGraphicsPipeline(render_pass, pipeline)
			sdl.BindGPUVertexBuffers(
				render_pass,
				0,
				&(sdl.GPUBufferBinding{buffer = vertex_buf}),
				1,
			)
			sdl.BindGPUIndexBuffer(render_pass, {buffer = index_buf}, ._16BIT)
			sdl.PushGPUVertexUniformData(cmd_buf, 0, &ubo, size_of(ubo))
			//sdl.DrawGPUPrimitives(render_pass, 3, 1, 0, 0)
			sdl.DrawGPUIndexedPrimitives(render_pass, 6, 1, 0 ,0, 0)
			sdl.EndGPURenderPass(render_pass)
		}

		ok = sdl.SubmitGPUCommandBuffer(cmd_buf); assert(ok)
	}
}
