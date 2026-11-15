package sdl3

SDL3_WASM_LIB :: #config(SDL3_WASM_LIB, "env.o")

when ODIN_ARCH == .wasm32 || ODIN_ARCH == .wasm64p32 {
	@(export) foreign import lib { SDL3_WASM_LIB }
} else when ODIN_OS == .Windows {
	@(export) foreign import lib { "SDL3.lib" }
} else when ODIN_OS == .Linux {
	@(export) foreign import lib { "CWD:build/libSDL3.so" }
} else {
	@(export) foreign import lib { "CWD:build/libSDL3.so" }
}
