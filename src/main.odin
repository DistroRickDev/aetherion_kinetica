package main

import "base:runtime"
import "core:fmt"
import "core:log"
import "core:mem"
import sdl "ffi/sdl3"
import "logger"
import "render"

EXAMPLE_FRAG_SHADER_PATH :: "/home/distrorickdev/projects/aetherion_kinetica/examples/shaders/glsl/shader.spv.frag"
EXAMPLE_VERT_SHADER_PATH :: "/home/distrorickdev/projects/aetherion_kinetica/examples/shaders/glsl/shader.spv.vert"

FRAG_SHADER_DATA := #load(EXAMPLE_FRAG_SHADER_PATH)
VERT_SHADER_DATA := #load(EXAMPLE_VERT_SHADER_PATH)

global_logger: log.Logger

main :: proc() {
	track: mem.Tracking_Allocator
	mem.tracking_allocator_init(&track, context.allocator)
	defer mem.tracking_allocator_destroy(&track)
	context.allocator = mem.tracking_allocator(&track)

	context.logger = global_logger
	logger.init_logger()
	defer (logger.destroy_logger())

	render.init({window_name = "Aetherion Kinetica", window_size = {1280, 780}, window_flags = {}})

	// TEST_TRIANGLE :: render.Vertices {
	// 	{pos = {-0.5, -0.5, 0}, color = {1, 0, 0, 1}},
	// 	{pos = {0, 0.5, 0}, color = {0, 1, 1, 1}},
	// 	{pos = {0.5, -0.5, 0}, color = {1, 0, 1, 1}},
	// }

	// render.set_render_buffer_data({TEST_TRIANGLE}, VERT_SHADER_DATA, FRAG_SHADER_DATA)

	TEST_QUAD :: render.Vertices {
		{pos = {-0.5, 0.5, 0}, color = {1, 0, 0, 1}}, // TL
		{pos = {0.5, 0.5, 0}, color = {0, 1, 1, 1}}, //TR
		{pos = {-0.5, -0.5, 0}, color = {1, 0, 1, 1}}, // BL
		{pos = {0.5, -0.5, 0}, color = {1, 0, 1, 1}}, // BR
	}

	TEST_QUAD_INDICES :: render.Indices {
		0,
		1,
		2, // TL -> TR -> BL
		2,
		1,
		3, // BL -> TR -> BR
	}

	render.set_render_buffer_data(
		{TEST_QUAD,TEST_QUAD_INDICES},
		VERT_SHADER_DATA,
		FRAG_SHADER_DATA,
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

	render.destroy_render_resources()

	if len(track.allocation_map) > 0 {
		for _, leak in track.allocation_map {
			fmt.eprintfln("'%v' leaked with size '%m'", leak.location, leak.size)
		}
		return
	}
	fmt.println("Exited with zero leaks")
}
