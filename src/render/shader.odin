package render

import sdl "../ffi/sdl3"
import spvc "../ffi/spirv_cross"
import "base:runtime"
import "core:c"
import "core:fmt"
import "core:slice"

Reflected_Resource_Info :: struct {
	id:           u32,
	base_type_id: u32,
	type_id:      u32,
	name:         string, // Cloned from spvc.Reflected_Resource.name so it outlives spvc_context
	location:     u32,
	set:          u32,
	binding:      u32,
	format:       sdl.GPUVertexElementFormat,
}

Shader_Resource :: struct {
	resources: [dynamic]Reflected_Resource_Info,
}

Shader_Object :: struct {
	shader_data:     []u8,
	shader_metadata: struct {
		inputs:           Shader_Resource,
		outputs:          Shader_Resource,
		uniform_buffers:  Shader_Resource,
		samplers:         Shader_Resource,
		storage_textures: Shader_Resource,
		storage_buffers:  Shader_Resource,
	},
}

@(private)
as_u32_buffer :: proc(code: []u8) -> []u32 {
	raw := transmute(runtime.Raw_Slice)code
	raw.len = len(code) / 4
	// raw.cap = len(bytes) / 4 // optional if capacity matters
	return transmute([]u32)raw
}

@(private)
to_reflected_resource_info :: proc(
	reflected_resource: ^spvc.Reflected_Resource,
	compiler: ^spvc.Compiler,
) -> Reflected_Resource_Info {
	return {
		id = reflected_resource.id,
		base_type_id = reflected_resource.id,
		type_id = reflected_resource.type_id,
		name = string(reflected_resource.name),
		location = spvc.compiler_get_decoration(
			compiler^,
			reflected_resource.id,
			spvc.Decoration.Decoration_Location,
		),
		set = spvc.compiler_get_decoration(
			compiler^,
			reflected_resource.id,
			spvc.Decoration.Decoration_DescriptorSet,
		),
		binding = spvc.compiler_get_decoration(
			compiler^,
			reflected_resource.id,
			spvc.Decoration.Decoration_Binding,
		),
		format = shader_resource_to_sdl_vertex_format(compiler^, reflected_resource.type_id),
	}
}

@(private)
to_shader_resource :: proc(
	reflected_resource: [^]spvc.Reflected_Resource,
	count: uint,
	compiler: ^spvc.Compiler,
) -> Shader_Resource {
	orig: []spvc.Reflected_Resource = slice.from_ptr(reflected_resource, int(count))
	dst := make([]Reflected_Resource_Info, len(orig))
	defer delete(dst)
	for &resource, i in orig {
		dst[i] = to_reflected_resource_info(&resource, compiler)
		// debug_print_resource(&dst[i])
	}
	return {resources = slice.clone_to_dynamic(dst)}
}

@(private)
debug_print_resource :: proc(resource_info: ^Reflected_Resource_Info) {
	fmt.printfln(
		"### Resource info: ###\nid:{}\nbase_type_id:{}\ntype_id:{}\nname:{}\nlocation:{}\nset:{}\nbinding:{}\nformat:{}\n######################",
		resource_info.id,
		resource_info.base_type_id,
		resource_info.type_id,
		resource_info.name,
		resource_info.location,
		resource_info.set,
		resource_info.binding,
		resource_info.format,
	)
}

@(private)
shader_resource_to_sdl_vertex_format :: proc(
	compiler: spvc.Compiler,
	resource_type_id: u32,
) -> sdl.GPUVertexElementFormat {
	type := spvc.compiler_get_type_handle(compiler, resource_type_id)
	base_type := spvc.type_get_basetype(type)
	vec_size := spvc.type_get_vector_size(type)

	#partial switch base_type {
	case .FP32:
		switch vec_size {
		case 1:
			return .FLOAT
		case 2:
			return .FLOAT2
		case 3:
			return .FLOAT3
		case 4:
			return .FLOAT4
		}
	case .INT32:
		switch vec_size {
		case 1:
			return .INT
		case 2:
			return .INT2
		case 3:
			return .INT3
		case 4:
			return .INT4
		}
	case .UINT32:
		switch vec_size {
		case 1:
			return .UINT
		case 2:
			return .UINT2
		case 3:
			return .UINT3
		case 4:
			return .UINT4
		}
	case .FP16:
		if vec_size == 2 {return .HALF2}
		if vec_size == 4 {return .HALF4}
	case:
		fmt.eprintfln("Unsuported format: {}", base_type)
	}
	return .INVALID
}

format_to_size :: proc(format: sdl.GPUVertexElementFormat) -> u32 {
	#partial switch format {
	case .INT:
		fallthrough
	case .UINT:
		fallthrough
	case .FLOAT:
		fallthrough
	case .BYTE4:
		fallthrough
	case .BYTE4_NORM:
		fallthrough
	case .UBYTE4:
		fallthrough
	case .UBYTE4_NORM:
		fallthrough
	case .SHORT2:
		fallthrough
	case .SHORT2_NORM:
		fallthrough
	case .USHORT2:
		fallthrough
	case .USHORT2_NORM:
		fallthrough
	case .HALF2:
		return 4

	case .INT2:
		fallthrough
	case .UINT2:
		fallthrough
	case .FLOAT2:
		fallthrough
	case .SHORT4:
		fallthrough
	case .SHORT4_NORM:
		fallthrough
	case .USHORT4:
		fallthrough
	case .USHORT4_NORM:
		fallthrough
	case .HALF4:
		return 8

	case .INT3:
		fallthrough
	case .UINT3:
		fallthrough
	case .FLOAT3:
		return 12

	case .INT4:
		fallthrough
	case .UINT4:
		fallthrough
	case .FLOAT4:
		return 16
	}
	return 0
}

get_shader_object_from_data :: proc(code: []u8) -> Shader_Object {
	ctx: spvc.Context
	err := spvc.context_create(&ctx); assert(err == .Success)
	defer spvc.context_destroy(ctx)

	ir: spvc.Parsed_IR
	data := as_u32_buffer(code)
	err = spvc.context_parse_spirv(ctx, raw_data(data), len(data), &ir); assert(err == .Success)

	compiler: spvc.Compiler
	err = spvc.context_create_compiler(ctx, .None, ir, .Copy, &compiler); assert(err == .Success)

	resources: spvc.Resources
	err = spvc.compiler_create_shader_resources(compiler, &resources); assert(err == .Success)

	uniform_buffers_raw: [^]spvc.Reflected_Resource
	num_uniform_buffers: c.size_t
	err = spvc.resources_get_resource_list_for_type(
		resources,
		.UNIFORM_BUFFER,
		&uniform_buffers_raw,
		&num_uniform_buffers,
	); assert(err == .Success)

	sampled_images_raw: [^]spvc.Reflected_Resource
	num_sampled_images: c.size_t
	err = spvc.resources_get_resource_list_for_type(
		resources,
		.SAMPLED_IMAGE,
		&sampled_images_raw,
		&num_sampled_images,
	); assert(err == .Success)

	storage_images_raw: [^]spvc.Reflected_Resource
	num_storage_images: c.size_t
	err = spvc.resources_get_resource_list_for_type(
		resources,
		.STORAGE_IMAGE,
		&storage_images_raw,
		&num_storage_images,
	); assert(err == .Success)

	storage_buffers_raw: [^]spvc.Reflected_Resource
	num_storage_buffers: c.size_t
	err = spvc.resources_get_resource_list_for_type(
		resources,
		.STORAGE_BUFFER,
		&storage_buffers_raw,
		&num_storage_buffers,
	); assert(err == .Success)


	inputs_raw: [^]spvc.Reflected_Resource
	num_of_inputs: c.size_t
	err = spvc.resources_get_resource_list_for_type(
		resources,
		.STAGE_INPUT,
		&inputs_raw,
		&num_of_inputs,
	); assert(err == .Success)


	outputs_raw: [^]spvc.Reflected_Resource
	num_of_outputs: c.size_t
	err = spvc.resources_get_resource_list_for_type(
		resources,
		.STAGE_OUTPUT,
		&outputs_raw,
		&num_of_outputs,
	); assert(err == .Success)

	return {
		shader_data = code,
		shader_metadata = {
			inputs = to_shader_resource(inputs_raw, num_of_inputs, &compiler),
			outputs = to_shader_resource(outputs_raw, num_of_outputs, &compiler),
			uniform_buffers = to_shader_resource(
				uniform_buffers_raw,
				num_uniform_buffers,
				&compiler,
			),
			samplers = to_shader_resource(sampled_images_raw, num_sampled_images, &compiler),
			storage_textures = to_shader_resource(
				storage_images_raw,
				num_storage_images,
				&compiler,
			),
			storage_buffers = to_shader_resource(
				storage_buffers_raw,
				num_storage_buffers,
				&compiler,
			),
		},
	}
}

load_shader :: proc(
	device: ^sdl.GPUDevice,
	shader_object: Shader_Object,
	stage: sdl.GPUShaderStage,
) -> ^sdl.GPUShader {
	return sdl.CreateGPUShader(
		device,
		{
			code_size = len(shader_object.shader_data),
			code = raw_data(shader_object.shader_data),
			entrypoint = "main",
			format = {.SPIRV},
			stage = stage,
			num_uniform_buffers = u32(
				len(shader_object.shader_metadata.uniform_buffers.resources),
			),
			num_samplers = u32(len(shader_object.shader_metadata.samplers.resources)),
			num_storage_buffers = u32(
				len(shader_object.shader_metadata.storage_buffers.resources),
			),
			num_storage_textures = u32(
				len(shader_object.shader_metadata.storage_textures.resources),
			),
		},
	)
}

clean_shader_object :: proc(shader_obj : ^Shader_Object) {
	delete(shader_obj.shader_metadata.inputs.resources)
	delete(shader_obj.shader_metadata.outputs.resources)
	delete(shader_obj.shader_metadata.uniform_buffers.resources)
	delete(shader_obj.shader_metadata.samplers.resources)
	delete(shader_obj.shader_metadata.storage_textures.resources)
	delete(shader_obj.shader_metadata.storage_buffers.resources)
}
