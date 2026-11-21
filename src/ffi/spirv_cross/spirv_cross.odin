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
Type :: distinct rawptr
// Reflected_Resource :: distinct rawptr

Reflected_Resource :: struct {
	id:           u32,
	base_type_id: u32,
	type_id:      u32,
	name:         cstring,
}

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
	UNKNOWN                = 0,
	UNIFORM_BUFFER         = 1,
	STORAGE_BUFFER         = 2,
	STAGE_INPUT            = 3,
	STAGE_OUTPUT           = 4,
	SUBPASS_INPUT          = 5,
	STORAGE_IMAGE          = 6,
	SAMPLED_IMAGE          = 7,
	ATOMIC_COUNTER         = 8,
	PUSH_CONSTANT          = 9,
	SEPARATE_IMAGE         = 10,
	SEPARATE_SAMPLERS      = 11,
	ACCELERATION_STRUCTURE = 12,
	RAY_QUERY              = 13,
	SHADER_RECORD_BUFFER   = 14,
	GL_PLAIN_UNIFORM       = 15,
	TENSOR                 = 16,
	INT_MAX                = 0x7fffffff,
}

Decoration :: enum c.int {
	Decoration_RelaxedPrecision                            = 0,
	Decoration_SpecId                                      = 1,
	Decoration_Block                                       = 2,
	Decoration_BufferBlock                                 = 3,
	Decoration_RowMajor                                    = 4,
	Decoration_ColMajor                                    = 5,
	Decoration_ArrayStride                                 = 6,
	Decoration_MatrixStride                                = 7,
	Decoration_GLSLShared                                  = 8,
	Decoration_GLSLPacked                                  = 9,
	Decoration_CPacked                                     = 10,
	Decoration_BuiltIn                                     = 11,
	Decoration_NoPerspective                               = 13,
	Decoration_Flat                                        = 14,
	Decoration_Patch                                       = 15,
	Decoration_Centroid                                    = 16,
	Decoration_Sample                                      = 17,
	Decoration_Invariant                                   = 18,
	Decoration_Restrict                                    = 19,
	Decoration_Aliased                                     = 20,
	Decoration_Volatile                                    = 21,
	Decoration_Constant                                    = 22,
	Decoration_Coherent                                    = 23,
	Decoration_NonWritable                                 = 24,
	Decoration_NonReadable                                 = 25,
	Decoration_Uniform                                     = 26,
	Decoration_UniformId                                   = 27,
	Decoration_SaturatedConversion                         = 28,
	Decoration_Stream                                      = 29,
	Decoration_Location                                    = 30,
	Decoration_Component                                   = 31,
	Decoration_Index                                       = 32,
	Decoration_Binding                                     = 33,
	Decoration_DescriptorSet                               = 34,
	Decoration_Offset                                      = 35,
	Decoration_XfbBuffer                                   = 36,
	Decoration_XfbStride                                   = 37,
	Decoration_FuncParamAttr                               = 38,
	Decoration_FPRoundingMode                              = 39,
	Decoration_FPFastMathMode                              = 40,
	Decoration_LinkageAttributes                           = 41,
	Decoration_NoContraction                               = 42,
	Decoration_InputAttachmentIndex                        = 43,
	Decoration_Alignment                                   = 44,
	Decoration_MaxByteOffset                               = 45,
	Decoration_AlignmentId                                 = 46,
	Decoration_MaxByteOffsetId                             = 47,
	Decoration_SaturatedToLargestFloat8NormalConversionEXT = 4216,
	Decoration_NoSignedWrap                                = 4469,
	Decoration_NoUnsignedWrap                              = 4470,
	Decoration_WeightTextureQCOM                           = 4487,
	Decoration_BlockMatchTextureQCOM                       = 4488,
	Decoration_BlockMatchSamplerQCOM                       = 4499,
	Decoration_ExplicitInterpAMD                           = 4999,
	Decoration_NodeSharesPayloadLimitsWithAMDX             = 5019,
	Decoration_NodeMaxPayloadsAMDX                         = 5020,
	Decoration_TrackFinishWritingAMDX                      = 5078,
	Decoration_PayloadNodeNameAMDX                         = 5091,
	Decoration_PayloadNodeBaseIndexAMDX                    = 5098,
	Decoration_PayloadNodeSparseArrayAMDX                  = 5099,
	Decoration_PayloadNodeArraySizeAMDX                    = 5100,
	Decoration_PayloadDispatchIndirectAMDX                 = 5105,
	Decoration_ArrayStrideIdEXT                            = 5124,
	Decoration_OffsetIdEXT                                 = 5125,
	Decoration_UTFEncodedKHR                               = 5145,
	Decoration_OverrideCoverageNV                          = 5248,
	Decoration_PassthroughNV                               = 5250,
	Decoration_ViewportRelativeNV                          = 5252,
	Decoration_SecondaryViewportRelativeNV                 = 5256,
	Decoration_PerPrimitiveEXT                             = 5271,
	Decoration_PerPrimitiveNV                              = 5271,
	Decoration_PerViewNV                                   = 5272,
	Decoration_PerTaskNV                                   = 5273,
	Decoration_PerVertexKHR                                = 5285,
	Decoration_PerVertexNV                                 = 5285,
	Decoration_NonUniform                                  = 5300,
	Decoration_NonUniformEXT                               = 5300,
	Decoration_RestrictPointer                             = 5355,
	Decoration_RestrictPointerEXT                          = 5355,
	Decoration_AliasedPointer                              = 5356,
	Decoration_AliasedPointerEXT                           = 5356,
	Decoration_MemberOffsetNV                              = 5358,
	Decoration_HitObjectShaderRecordBufferNV               = 5386,
	Decoration_HitObjectShaderRecordBufferEXT              = 5389,
	Decoration_BankNV                                      = 5397,
	Decoration_BindlessSamplerNV                           = 5398,
	Decoration_BindlessImageNV                             = 5399,
	Decoration_BoundSamplerNV                              = 5400,
	Decoration_BoundImageNV                                = 5401,
	Decoration_SIMTCallINTEL                               = 5599,
	Decoration_ReferencedIndirectlyINTEL                   = 5602,
	Decoration_ClobberINTEL                                = 5607,
	Decoration_SideEffectsINTEL                            = 5608,
	Decoration_VectorComputeVariableINTEL                  = 5624,
	Decoration_FuncParamIOKindINTEL                        = 5625,
	Decoration_VectorComputeFunctionINTEL                  = 5626,
	Decoration_StackCallINTEL                              = 5627,
	Decoration_GlobalVariableOffsetINTEL                   = 5628,
	Decoration_CounterBuffer                               = 5634,
	Decoration_HlslCounterBufferGOOGLE                     = 5634,
	Decoration_HlslSemanticGOOGLE                          = 5635,
	Decoration_UserSemantic                                = 5635,
	Decoration_UserTypeGOOGLE                              = 5636,
	Decoration_FunctionRoundingModeINTEL                   = 5822,
	Decoration_FunctionDenormModeINTEL                     = 5823,
	Decoration_RegisterALTERA                              = 5825,
	Decoration_RegisterINTEL                               = 5825,
	Decoration_MemoryALTERA                                = 5826,
	Decoration_MemoryINTEL                                 = 5826,
	Decoration_NumbanksALTERA                              = 5827,
	Decoration_NumbanksINTEL                               = 5827,
	Decoration_BankwidthALTERA                             = 5828,
	Decoration_BankwidthINTEL                              = 5828,
	Decoration_MaxPrivateCopiesALTERA                      = 5829,
	Decoration_MaxPrivateCopiesINTEL                       = 5829,
	Decoration_SinglepumpALTERA                            = 5830,
	Decoration_SinglepumpINTEL                             = 5830,
	Decoration_DoublepumpALTERA                            = 5831,
	Decoration_DoublepumpINTEL                             = 5831,
	Decoration_MaxReplicatesALTERA                         = 5832,
	Decoration_MaxReplicatesINTEL                          = 5832,
	Decoration_SimpleDualPortALTERA                        = 5833,
	Decoration_SimpleDualPortINTEL                         = 5833,
	Decoration_MergeALTERA                                 = 5834,
	Decoration_MergeINTEL                                  = 5834,
	Decoration_BankBitsALTERA                              = 5835,
	Decoration_BankBitsINTEL                               = 5835,
	Decoration_ForcePow2DepthALTERA                        = 5836,
	Decoration_ForcePow2DepthINTEL                         = 5836,
	Decoration_StridesizeALTERA                            = 5883,
	Decoration_StridesizeINTEL                             = 5883,
	Decoration_WordsizeALTERA                              = 5884,
	Decoration_WordsizeINTEL                               = 5884,
	Decoration_TrueDualPortALTERA                          = 5885,
	Decoration_TrueDualPortINTEL                           = 5885,
	Decoration_BurstCoalesceALTERA                         = 5899,
	Decoration_BurstCoalesceINTEL                          = 5899,
	Decoration_CacheSizeALTERA                             = 5900,
	Decoration_CacheSizeINTEL                              = 5900,
	Decoration_DontStaticallyCoalesceALTERA                = 5901,
	Decoration_DontStaticallyCoalesceINTEL                 = 5901,
	Decoration_PrefetchALTERA                              = 5902,
	Decoration_PrefetchINTEL                               = 5902,
	Decoration_StallEnableALTERA                           = 5905,
	Decoration_StallEnableINTEL                            = 5905,
	Decoration_FuseLoopsInFunctionALTERA                   = 5907,
	Decoration_FuseLoopsInFunctionINTEL                    = 5907,
	Decoration_MathOpDSPModeALTERA                         = 5909,
	Decoration_MathOpDSPModeINTEL                          = 5909,
	Decoration_AliasScopeINTEL                             = 5914,
	Decoration_NoAliasINTEL                                = 5915,
	Decoration_InitiationIntervalALTERA                    = 5917,
	Decoration_InitiationIntervalINTEL                     = 5917,
	Decoration_MaxConcurrencyALTERA                        = 5918,
	Decoration_MaxConcurrencyINTEL                         = 5918,
	Decoration_PipelineEnableALTERA                        = 5919,
	Decoration_PipelineEnableINTEL                         = 5919,
	Decoration_BufferLocationALTERA                        = 5921,
	Decoration_BufferLocationINTEL                         = 5921,
	Decoration_IOPipeStorageALTERA                         = 5944,
	Decoration_IOPipeStorageINTEL                          = 5944,
	Decoration_FunctionFloatingPointModeINTEL              = 6080,
	Decoration_SingleElementVectorINTEL                    = 6085,
	Decoration_VectorComputeCallableFunctionINTEL          = 6087,
	Decoration_MediaBlockIOINTEL                           = 6140,
	Decoration_StallFreeALTERA                             = 6151,
	Decoration_StallFreeINTEL                              = 6151,
	Decoration_FPMaxErrorDecorationINTEL                   = 6170,
	Decoration_LatencyControlLabelALTERA                   = 6172,
	Decoration_LatencyControlLabelINTEL                    = 6172,
	Decoration_LatencyControlConstraintALTERA              = 6173,
	Decoration_LatencyControlConstraintINTEL               = 6173,
	Decoration_ConduitKernelArgumentALTERA                 = 6175,
	Decoration_ConduitKernelArgumentINTEL                  = 6175,
	Decoration_RegisterMapKernelArgumentALTERA             = 6176,
	Decoration_RegisterMapKernelArgumentINTEL              = 6176,
	Decoration_MMHostInterfaceAddressWidthALTERA           = 6177,
	Decoration_MMHostInterfaceAddressWidthINTEL            = 6177,
	Decoration_MMHostInterfaceDataWidthALTERA              = 6178,
	Decoration_MMHostInterfaceDataWidthINTEL               = 6178,
	Decoration_MMHostInterfaceLatencyALTERA                = 6179,
	Decoration_MMHostInterfaceLatencyINTEL                 = 6179,
	Decoration_MMHostInterfaceReadWriteModeALTERA          = 6180,
	Decoration_MMHostInterfaceReadWriteModeINTEL           = 6180,
	Decoration_MMHostInterfaceMaxBurstALTERA               = 6181,
	Decoration_MMHostInterfaceMaxBurstINTEL                = 6181,
	Decoration_MMHostInterfaceWaitRequestALTERA            = 6182,
	Decoration_MMHostInterfaceWaitRequestINTEL             = 6182,
	Decoration_StableKernelArgumentALTERA                  = 6183,
	Decoration_StableKernelArgumentINTEL                   = 6183,
	Decoration_HostAccessINTEL                             = 6188,
	Decoration_InitModeALTERA                              = 6190,
	Decoration_InitModeINTEL                               = 6190,
	Decoration_ImplementInRegisterMapALTERA                = 6191,
	Decoration_ImplementInRegisterMapINTEL                 = 6191,
	Decoration_ConditionalINTEL                            = 6247,
	Decoration_CacheControlLoadINTEL                       = 6442,
	Decoration_CacheControlStoreINTEL                      = 6443,
	Decoration_Max                                         = 0x7fffffff,
}

Base_Type :: enum c.int {
	UNKNOWN                = 0,
	VOID                   = 1,
	BOOLEAN                = 2,
	INT8                   = 3,
	UINT8                  = 4,
	INT16                  = 5,
	UINT16                 = 6,
	INT32                  = 7,
	UINT32                 = 8,
	INT64                  = 9,
	UINT64                 = 10,
	ATOMIC_COUNTER         = 11,
	FP16                   = 12,
	FP32                   = 13,
	FP64                   = 14,
	STRUCT                 = 15,
	IMAGE                  = 16,
	SAMPLED_IMAGE          = 17,
	SAMPLER                = 18,
	ACCELERATION_STRUCTURE = 19,
	INT_MAX                = 0x7fffffff,
}

@(default_calling_convention = "c", link_prefix = "spvc_")
foreign spvc {
	context_create :: proc(ctx: ^Context) -> Result ---
	context_destroy :: proc(ctx: Context) ---
	context_parse_spirv :: proc(ctx: Context, spirv: ^u32, word_count: c.size_t, parsed_ir: ^Parsed_IR) -> Result ---

	context_create_compiler :: proc(ctx: Context, backend: Backend, parsed_ir: Parsed_IR, mode: Capture_Mode, compiler: ^Compiler) -> Result ---

	compiler_compile :: proc(compiler: Compiler, source: ^cstring) -> Result ---

	compiler_create_shader_resources :: proc(compiler: Compiler, resources: ^Resources) -> Result ---

	resources_get_resource_list_for_type :: proc(resources: Resources, type_ubo: Resource_Type_Uniform_Buffer, resource: ^[^]Reflected_Resource, count: ^c.size_t) -> Result ---

	compiler_get_decoration :: proc(compiler: Compiler, id: c.uint, decoration: Decoration) -> c.uint ---

	compiler_get_type_handle :: proc(compiler: Compiler, id: c.uint) -> Type ---

	type_get_base_type_id :: proc(type: Type) -> c.uint ---

	type_get_basetype :: proc(type: Type) -> Base_Type ---

	type_get_vector_size :: proc(type: Type) -> c.uint ---
}
