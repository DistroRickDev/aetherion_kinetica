package main

import "base:runtime"
import "core:fmt"
import "core:mem"
import "logger"
import "render"
import sdl "vendor:sdl3"

EXAMPLE_FRAG_SHADER_PATH :: "/home/distrorickdev/projects/aetherion_kinetica/examples/shaders/glsl/shader.spv.frag"
EXAMPLE_VERT_SHADER_PATH :: "/home/distrorickdev/projects/aetherion_kinetica/examples/shaders/glsl/shader.spv.vert"

FRAG_SHADER_DATA := #load(EXAMPLE_FRAG_SHADER_PATH)
VERT_SHADER_DATA := #load(EXAMPLE_VERT_SHADER_PATH)

main :: proc() {
	track: mem.Tracking_Allocator
	mem.tracking_allocator_init(&track, context.allocator)
	defer mem.tracking_allocator_destroy(&track)
	context.allocator = mem.tracking_allocator(&track)

	logger.init_logger()
	defer (logger.destroy_logger())

	render.init({window_name = "Hello SDL3", window_size = {1280, 780}, window_flags = {}})
	defer (render.destroy_render_resources())

	TEST_TRIANGLE :: []render.VertexData {
		{pos = {-0.5, -0.5, 0}, color = {1, 0, 0, 1}},
		{pos = {0, 0.5, 0}, color = {0, 1, 1, 1}},
		{pos = {0.5, -0.5, 0}, color = {1, 0, 1, 1}},
	}

	render.set_render_primitive(
		TEST_TRIANGLE,
		{
			shader_data = VERT_SHADER_DATA,
			shader_metadata = {
				num_of_samplers = 0,
				storage_textures = 0,
				storage_buffers = 0,
				num_uniform_buffers = 1,
			},
		},
		{
			shader_data = FRAG_SHADER_DATA,
			shader_metadata = {
				num_of_samplers = 0,
				storage_textures = 0,
				storage_buffers = 0,
				num_uniform_buffers = 0,
			},
		},
	)
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
		render.render_frame(delta_time)
	}

	if len(track.allocation_map) > 0 {
		for _, leak in track.allocation_map {
			fmt.eprintfln("'%v' leaked with size '%m'", leak.location, leak.size)
		}
		return
	}
	fmt.println("Exited with zero leaks")
}
