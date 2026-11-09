package render

import sdl "vendor:sdl3"

WindowSettings :: struct {
	window_size:  [2]i32,
	window_name:  cstring,
	window_flags: sdl.WindowFlags,
}

@(private)
main_window: ^sdl.Window

get_window :: proc() -> ^sdl.Window {
	assert(main_window != nil)
	return main_window
}

@(private)
gpu: ^sdl.GPUDevice

get_device :: proc() -> ^sdl.GPUDevice {
	assert(gpu != nil)
	return gpu
}

init :: proc(settings: WindowSettings) {
	main_window = sdl.CreateWindow(
		settings.window_name,
		settings.window_size.x,
		settings.window_size.y,
		settings.window_flags,
	); assert(main_window != nil)
	gpu = sdl.CreateGPUDevice({.SPIRV}, true, nil); assert(gpu != nil)
}

destroy_gpu_resources :: proc() {
	ok:= sdl.WaitForGPUIdle(gpu); assert(ok)
	sdl.DestroyGPUDevice(gpu)
	sdl.DestroyWindow(main_window)
}
