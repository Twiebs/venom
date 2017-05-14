#define VENOM_SINGLE_TRANSLATION_UNIT
//#define VENOM_HOTLOAD
#define VENOM_OPENGL
#define VENOM_SINGLE_THREADED

#define VENOM_TRACK_MEMORY 1
#define VENOM_TRACK_LOCKS 1

#define VENOM_DEFAULT_SCREEN_WIDTH  1792
#define VENOM_DEFAULT_SCREEN_HEIGHT 1008
//#define VENOM_DEFAULT_SCREEN_WIDTH  1600
//#define VENOM_DEFAULT_SCREEN_HEIGHT 900

#define VENOM_USER_CONFIG_PATH "userdata"
#define VENOM_MODULE_SOURCE_FILENAME "test_scene.cpp"
#define VENOM_MODULE_FILENAME "../build/game_module.so"

#define VENOM_ASSET_DIRECTORY "../assets/"

#define VENOM_SOUCE_FILE(file) "../src" file
#define VENOM_SHADER_FILE(file) "../src/shaders/" file
#define VENOM_ASSET_FILE(file) "../assets/" file

#define VenomConstantList                                                      \
  _(U32, SHADOW_MAP_RESOLUTION, 4096)                                          \
  _(U32, SHADOW_MAP_CASCADE_COUNT, 8)                                          \
  _(F32, SHADOW_MAP_CASCADE_WEIGHT, 0.75)                                      \
  _(F32, SHADOW_MAP_CASCADE_TOLERANCE, 1.005f)                                 \
  _(U32, TERRAIN_MATERIAL_COUNT, 2)                                            \
  _(F32, TERRAIN_HEIGHT_SCALAR, 32.0f)                                         \
  _(U32, DIRECTIONAL_LIGHTS_MAX, 1)                                            \
  _(U32, SHADOW_CASTING_POINT_LIGHT_MAX, 8)                                    \
  _(U32, POINT_LIGHTS_MAX, 16)                                                 \
  _(U32, DIRECTIONAL_LIGHT_UNIFORM_LOCATION, 8) \
  _(U32, SSAO_SAMPLE_COUNT, 64) \
  _(U32, SSAO_NOISE_SIZE, 4)

static const U32 UNIFORM_COUNT_PER_DIRECTIONAL_LIGHT = 2;
static const U32 UNIFORM_COUNT_PER_POINT_LIGHT = 3;

#define VenomShaderCalculatedValueList                                         \
  _(U32, POINT_LIGHT_UNIFORM_LOCATION,                                         \
    DIRECTIONAL_LIGHT_UNIFORM_LOCATION +                                       \
        DIRECTIONAL_LIGHTS_MAX * UNIFORM_COUNT_PER_DIRECTIONAL_LIGHT)          \
  _(U32, SHADOW_CASTING_POINT_LIGHT_UNIFORM_LOCATION,                          \
    POINT_LIGHT_UNIFORM_LOCATION +                                             \
        POINT_LIGHTS_MAX * UNIFORM_COUNT_PER_POINT_LIGHT)

#define _(type,name,value) static const type name = value;
VenomConstantList
VenomShaderCalculatedValueList
#undef _

static const char* GLSL_SHADER_HEADER {
	"#version 430 core\n"
	"#extension GL_ARB_explicit_uniform_location : require\n"
	"#extension GL_ARB_shading_language_420pack : require\n\n"
	#define _(type,name,value) "#define " #name " " #value "\n"
	VenomConstantList
	#undef _
  "\n"
};