#define VENOM_SINGLE_TRANSLATION_UNIT
#define VENOM_HOTLOAD
#define VENOM_PROFILER 

#define VENOM_HOTLOAD_MODULE_FILE "main.cpp"
#define VENOM_SOUCE_FILE(file) "../src" file
#define VENOM_SHADER_FILE(file) "../src/shaders/" file
#define VENOM_ASSET_FILE(file) "../assets/" file

#define VenomConstantList						 \
	_(U32, SHADOW_MAP_RESOLUTION, 4096)			 \
	_(U32, SHADOW_MAP_CASCADE_COUNT, 1)		 	 \
	_(F32, SHADOW_MAP_CASCADE_WEIGHT, 0.75)		 \
	_(F32, SHADOW_MAP_CASCADE_TOLERANCE, 1.005f) \
	_(U32, TERRAIN_MATERIAL_COUNT, 2)			 \
	_(F32, TERRAIN_HEIGHT_SCALAR, 32.0f)		 \



#define _(type,name,value) static const type name = value;
VenomConstantList
#undef _

static const char* GLSL_SHADER_HEADER 
{
	"#version 330 core\n"
	"#extension GL_ARB_explicit_uniform_location : require\n"
	"#extension GL_ARB_shading_language_420pack : require\n"
	#define _(type,name,value) "#define " #name " " #value "\n"
	VenomConstantList
	#undef _
};


