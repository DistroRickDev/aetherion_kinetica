package build

import "core:encoding/json"
import "core:log"
import "core:os"
import "core:path/filepath"
import "core:slice"
import "core:strings"

BUILD_DIR :: "build"
SDL_BUID_DIR :: "/external/SDL"

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
}

build_sdl3 :: proc() {
	log.debug("Attempting to build SDL")
	run_critical_process({"cmake", "--build", BUILD_DIR, "--target", "SDL3-shared", "-j16"})
}

run_external_libs_cmake :: proc() {
	if !os.is_directory(BUILD_DIR) {
		log.warnf("Build directory {} does not exist creating it ...", BUILD_DIR)
		if os.make_directory(BUILD_DIR) != nil {
			log.errorf("Failed to create {}, exiting ...", BUILD_DIR)

		}
	}
	log.debug("Attempting to build external libraries inside directory {} ...", BUILD_DIR)
	run_critical_process({"cmake", "-DSDL_SHARED=ON", "-B", BUILD_DIR, "-S", "."})
}

compile_using_glsl :: proc(path: string) {
	out_file, success := strings.replace_all(path, ".glsl", ".spv"); assert(success)
	run_critical_process({"glslc", path, "-o", out_file})
}

compile_shaders :: proc(path: ^string) {
	files,err := os.read_all_directory_by_path(path^, allocator= context.temp_allocator); assert(err == nil)
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
	BUILD_COMMAND_STR :: "odin build src -debug -out:" + OUT + " -error-pos-style:unix"
	build_cmd := strings.split(BUILD_COMMAND_STR, " ")
	// Needs to be built outside splitted string otherwise -Wl is parsed without the ' ' char
	SDL_LIB_PATH_FLAG :: "-L./" + BUILD_DIR + SDL_BUID_DIR + " -Wl"
	EXTRA_LINKER_FLAGS :: "-extra-linker-flags:" + `"` + SDL_LIB_PATH_FLAG + `"`
	full_build_cmd := make([]string, len(build_cmd) + 1)
	defer (delete(full_build_cmd))
	copy(full_build_cmd, build_cmd)
	full_build_cmd[len(build_cmd)] = EXTRA_LINKER_FLAGS
	run_critical_process(full_build_cmd)
}

BuildConfig :: struct {
	run_cmake: bool,
	build_flags:   struct {
		build_sdl:         bool,
		build_spriv_cross: bool,
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
	// Clean up your permanently cloned strings when main exits!
	defer free_build_config(&build_config)

	if build_config.run_cmake {
		log.debug("Configured to run external lib cmake generation")
		run_external_libs_cmake()
	}

	if build_config.build_flags.build_sdl {
		log.debug("Configured to build SDL from scratch")
		build_sdl3()
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
			build_spriv_cross: {},
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
		build_config.build_flags.build_spriv_cross,
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
	err := json.unmarshal(BUILD_CONFIG_RAW, &tmp, allocator= context.temp_allocator); assert(nil == err)
	return {
		build_flags = tmp.build_flags,
		build_command = strings.clone(tmp.build_command),
		shader_config = {
			compile_shaders = tmp.shader_config.compile_shaders,
			path_from_project_root = strings.clone(tmp.shader_config.path_from_project_root),
			format = strings.clone(tmp.shader_config.format)
		}
	}
}

// Helper to prevent memory leaks from the cloned heap strings
free_build_config :: proc(config: ^BuildConfig) {
	delete(config.build_command)
	delete(config.shader_config.path_from_project_root)
	delete(config.shader_config.format)
}
