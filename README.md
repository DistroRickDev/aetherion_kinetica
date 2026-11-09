# Aetherion Kinetica

Is an Half-Life 2  style based, basic 3D game engine that aims to bring modern graphics (using SDL3 GPU API) with a custom physics engine, that aims to develop games with Half-Life 2 style.

## Roadmap
The roadmap can be found [here](./docs/roadmap.md)

## Requirements
1. odin

## Build instructions

```bash
    odin run .
```

## Basic configuration (build_config.json)
```json
{
  "run_cmake": false,
  "build_flags": {
    "build_sdl": false,
    "build_spriv_cross": true,
    "build_shader_c": true,
    "build_im_gui": true
  },
  "build_command": "build", // currently only supported command
  "shader_config": {
    "compile_shaders": true,
    "path_from_project_root": "examples/shaders/glsl",
    "format": "glsl" // currently only supported source shader language
  }
}
```

## Author
[@DistroRickDev](https://github.com/DistroRickDev)

## License
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)
