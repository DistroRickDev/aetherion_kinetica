package render

import sdl "../ffi/sdl3"

Vec3 :: [3]f32
Vec2 :: [2]f32

Vertex :: struct {
	pos:   Vec3,
	color: sdl.FColor,
	uv: Vec2
}

Vertices :: []Vertex

get_vertex_buffer_size :: proc(vertices: Vertices) -> u32 {
	vertices_len := len(vertices)
	assert(vertices_len > 0)
	return u32(vertices_len * size_of(vertices[0]))
}

Indices :: []u16

get_indices_size :: proc(indices: Indices) -> u32 {
	indices_len := len(indices)
	assert(indices_len > 0)
	return u32(indices_len * size_of(indices[0]))
}
