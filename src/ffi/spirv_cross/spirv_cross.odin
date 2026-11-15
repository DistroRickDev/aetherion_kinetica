package spirv_cross

import "core:c"

when ODIN_OS == .Windows {
	foreign import spvc "CWD:build/spirv-cross-c-shared.lib" // Or static libs
} else when ODIN_OS == .Linux {
	foreign import spvc "CWD:build/libspirv-cross-c-shared.so"
}

Context :: distinct rawptr
Parsed_IR :: distinct rawptr
Compiler :: distinct rawptr
Compiler_Options :: distinct rawptr
Resources :: distinct rawptr
Reflected_Resource :: distinct rawptr

Result :: enum c.int {
	Success                 = 0,
	Error_Invalid_SPIRV     = -1,
	Error_Unsupported_SPIRV = -2,
	Error_Out_Of_Memory     = -3,
	Error_Invalid_Argument  = -4,
}

Backend :: enum c.int {
	None = 0,
	GLSL = 1,
	HLSL = 2,
	MSL  = 3,
	CPP  = 4,
	JSON = 5,
}

Capture_Mode :: enum c.int {
	Copy           = 0,
	Take_Ownership = 1,
}

Resource_Type_Uniform_Buffer :: enum c.int {
	UNKNOWN = 0,
	UNIFORM_BUFFER = 1,
	STORAGE_BUFFER = 2,
	STAGE_INPUT = 3,
	STAGE_OUTPUT = 4,
	SUBPASS_INPUT = 5,
	STORAGE_IMAGE = 6,
	SAMPLED_IMAGE = 7,
	ATOMIC_COUNTER = 8,
	PUSH_CONSTANT = 9,
	SEPARATE_IMAGE = 10,
	SEPARATE_SAMPLERS = 11,
	ACCELERATION_STRUCTURE = 12,
	RAY_QUERY = 13,
	SHADER_RECORD_BUFFER = 14,
	GL_PLAIN_UNIFORM = 15,
	TENSOR = 16,
	INT_MAX = 0x7fffffff
}

@(default_calling_convention = "c", link_prefix = "spvc_")
foreign spvc {
	context_create :: proc(ctx: ^Context) -> Result ---
	context_destroy :: proc(ctx: Context) ---
	context_parse_spirv :: proc(ctx: Context, spirv: ^u32, word_count: c.size_t, parsed_ir: ^Parsed_IR) -> Result ---

	context_create_compiler :: proc(ctx: Context, backend: Backend, parsed_ir: Parsed_IR, mode: Capture_Mode, compiler: ^Compiler) -> Result ---

	compiler_compile :: proc(compiler: Compiler, source: ^cstring) -> Result ---

	compiler_create_shader_resources :: proc(compiler: Compiler, resources: ^Resources) -> Result ---

	resources_get_resource_list_for_type :: proc(resources: Resources, type_ubo: Resource_Type_Uniform_Buffer, resource: ^Reflected_Resource, count: ^c.size_t) -> Result ---
}
