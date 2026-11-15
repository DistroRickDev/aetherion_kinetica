package build

import "base:runtime"
import "core:encoding/json"
import "core:fmt"
import "core:log"
import "core:os"
import "core:path/filepath"
import "core:slice"
import "core:strings"

BUILD_DIR :: "build"
SDL_BUID_DIR :: "/external/SDL"
SPIRV_CROSS_BUILD_DIR :: "/external/SPIRV-Cross"

run_critical_process :: proc(cmd: []string) {
	process, err_start := os.process_start(
		{command = cmd, stdin = os.stdin, stdout = os.stdout, stderr = os.stderr},
	)
	if nil != err_start {
		log.errorf("Failed to start process from command: '{}'", cmd)
		os.exit(-1)
	}
	state, err_wait := os.process_wait(process)
	if nil != err_wait {
		log.errorf("Failed to wait process from command: '{}'", cmd)
		os.exit(-1)
	}
	log.debugf("Commands '{}' process, terminated with error code: {}", cmd, state.exit_code)
	assert(state.exit_code == 0)
}

build_sdl3 :: proc() {
	log.debug("Attempting to build SDL")
	run_critical_process({"cmake", "--build", BUILD_DIR, "--target", "SDL3-shared", "-j16"})
	run_critical_process(
		{
			"cp",
			"-r",
			BUILD_DIR + SDL_BUID_DIR + "/libSDL3.so.0.5.0",
			BUILD_DIR + SDL_BUID_DIR + "/libSDL3.so",
			BUILD_DIR + SDL_BUID_DIR + "/libSDL3.so.0",
			BUILD_DIR,
		},
	)
}

build_spirv_cross :: proc() {
	log.debug("Attempting to build SDL")
	run_critical_process({"cmake", "--build", BUILD_DIR, "--target", "spirv-cross", "-j16"})
	run_critical_process(
		{"cmake", "--build", BUILD_DIR, "--target", "spirv-cross-c-shared", "-j16"},
	)
	run_critical_process(
		{
			"cp",
			"-r",
			BUILD_DIR + SPIRV_CROSS_BUILD_DIR + "/libspirv-cross-c-shared.so.0.68.0",
			BUILD_DIR + SPIRV_CROSS_BUILD_DIR + "/libspirv-cross-c-shared.so.0",
			BUILD_DIR + SPIRV_CROSS_BUILD_DIR + "/libspirv-cross-c-shared.so",
			BUILD_DIR,
		},
	)
}

run_external_libs_cmake :: proc(cmake_flags: string) {
	if !os.is_directory(BUILD_DIR) {
		log.warnf("Build directory {} does not exist creating it ...", BUILD_DIR)
		if os.make_directory(BUILD_DIR) != nil {
			log.errorf("Failed to create {}, exiting ...", BUILD_DIR)

		}
	}

	cmake_command := fmt.tprintf("cmake {} -B {} -S .", cmake_flags, BUILD_DIR)
	log.debug("Attempting to build external libraries inside directory {} ...", BUILD_DIR)
	run_critical_process(strings.split(cmake_command, " "))
}

compile_using_glsl :: proc(path: string) {
	out_file, success := strings.replace_all(path, ".glsl", ".spv"); assert(success)
	run_critical_process({"glslc", path, "-o", out_file})
}

compile_shaders :: proc(path: ^string) {
	files, err := os.read_all_directory_by_path(
		path^,
		allocator = context.temp_allocator,
	); assert(err == nil)
	for file in files {
		if strings.contains(file.name, ".glsl") {
			log.debugf("Found .glsl source file: '{}' compiling it", file.name)
			compile_using_glsl(file.fullpath)
		}
	}
}

build_engine :: proc() {
	EXE :: BUILD_DIR + "/aetherion_kinetica_engine"
	OUT :: EXE + ".exe" when ODIN_OS == .Windows else EXE
	project_root_dir, err := os.get_working_directory(
		context.allocator,
	); assert(err == os.General_Error.None)
	defer (delete(project_root_dir))
	BUILD_COMMAND_STR := fmt.tprintf(
		"odin build  src -debug -out:{} -error-pos-style:unix -collection:CWD={}",
		OUT,
		project_root_dir,
	)

	run_critical_process(strings.split(BUILD_COMMAND_STR, " "))
}

BuildConfig :: struct {
	run_cmake:     bool,
	build_flags:   struct {
		build_sdl:         bool,
		build_spirv_cross: bool,
		build_shader_c:    bool,
		build_im_gui:      bool,
	},
	build_command: string,
	shader_config: struct {
		compile_shaders:        bool,
		path_from_project_root: string,
		format:                 string,
	},
}

main :: proc() {
	context.logger = log.create_console_logger()
	// CRITICAL: Initialize / reset the temporary allocator frame so it's clean and safe to use
	defer free_all(context.temp_allocator)
	build_config := parse_build_config()
	print_debug_build_config(&build_config)
	// Clean up your permanently cloned strings when main exits!
	defer free_build_config(&build_config)

	if build_config.build_flags.build_sdl {
		log.debug("Configured to build SDL from scratch")
		if build_config.run_cmake {
			run_external_libs_cmake("-DSDL_SHARED=ON")
		}
		build_sdl3()
	}

	if build_config.build_flags.build_spirv_cross {
		log.debug("Configured to build Spirv-cross from scratch")
		if build_config.run_cmake {
			run_external_libs_cmake("-DSPIRV_CROSS_SHARED=ON")
		}
		build_spirv_cross()
	}

	if build_config.shader_config.compile_shaders {
		log.debug("Configured to compile shaders from GLSL to SPIRV")
		compile_shaders(&build_config.shader_config.path_from_project_root)
	}

	if build_config.build_command == "build" {
		log.debug("Configured to build the engine")
		build_engine()
	}
}

print_debug_build_config :: proc(build_config: ^BuildConfig) {
	log.debugf(
		"""

		########## Build config ##########
		run_cmake: {},

		build_flags:
			build_sdl: {},
			build_spirv_cross: {},
			build_shader_c: {},
			build_im_gui: {},

		build_command: {}

		shader_config:
			compile_shaders: {},
			path_from_project_root: {},
			format: {}

		########## End Build config ##########
		""",
		build_config.run_cmake,
		build_config.build_flags.build_sdl,
		build_config.build_flags.build_spirv_cross,
		build_config.build_flags.build_shader_c,
		build_config.build_flags.build_im_gui,
		build_config.build_command,
		build_config.shader_config.compile_shaders,
		build_config.shader_config.path_from_project_root,
		build_config.shader_config.path_from_project_root,
	)
}

BUILD_CONFIG_RAW :: #load("build_config.json")
parse_build_config :: proc() -> BuildConfig {
	tmp: BuildConfig
	err := json.unmarshal(
		BUILD_CONFIG_RAW,
		&tmp,
		allocator = context.temp_allocator,
	); assert(nil == err)
	return {
		run_cmake = tmp.run_cmake,
		build_flags = tmp.build_flags,
		build_command = strings.clone(tmp.build_command),
		shader_config = {
			compile_shaders = tmp.shader_config.compile_shaders,
			path_from_project_root = strings.clone(tmp.shader_config.path_from_project_root),
			format = strings.clone(tmp.shader_config.format),
		},
	}
}

// Helper to prevent memory leaks from the cloned heap strings
free_build_config :: proc(config: ^BuildConfig) {
	delete(config.build_command)
	delete(config.shader_config.path_from_project_root)
	delete(config.shader_config.format)
}
