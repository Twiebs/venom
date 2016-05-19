#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t   S8;
typedef int16_t  S16;
typedef int32_t  S32;
typedef int64_t  S64;
typedef int8_t   B8;
typedef int32_t  B32;
typedef float    F32;
typedef double   F64;

#define KILOBYTES(x) ((x) * 1024LL)
#define MEGABYTES(x) (KILOBYTES(x) * 1024LL)
#define GIGABYTES(x) (MEGABYTES(x) * 1024LL)
#define ARRAY_COUNT(array) (sizeof(array) / sizeof(*array))
#define STATIC_STRLEN(string) ((sizeof(string)) - 1)
#define static_strlen(string) ((sizeof(string)) - 1)
#define strict_assert(expr) assert(expr)
#define StrictAssert(expr) assert(expr)
#define Assert(expr) assert(expr)

#include "venom_config.h"

//Configuration Post-Processing
#ifdef VENOM_RELEASE
#undef VENOM_HOTLOAD
#undef VENOM_PROFILER
#endif

#include "venom_memory.h"
#include "venom_math.h"
#include "venom_audio.h"
#include "venom_render.h"
#include "venom_physics.h"
#include "venom_asset.h"

#ifndef VENOM_RELEASE
#include "venom_debug.h"
#include "imgui.h"

#define EngineDEBUGAPI \
  _(U64, GetPerformanceCounterTime) \
  _(U64, GetPerformanceCounterFrequency) \
  _(U64, GetFileLastWriteTime, const char *)  \
  _(DebugMemory*, GetDebugMemory)

#else //!VENOM_RELEASE
#define EngineDEBUGAPI
#endif

#define EngineAPIList				              \
  _(GLuint, GetShaderProgram, DEBUGShaderID, GameAssets*) \
  _(const ModelDrawable&, GetModelDrawable, DEBUGModelID, GameAssets*)\
  _(const MaterialDrawable&, GetMaterial, U32, GameAssets*)\
  EngineDEBUGAPI
#ifdef VENOM_HOTLOAD
#define _(returnType, name, ...) typedef returnType (*name##Proc)(__VA_ARGS__);
EngineAPIList
#undef _
#else
#define _(rettype, name, ...) rettype name(__VA_ARGS__);
EngineAPIList
#undef _
#endif

#define fori(lim) for(int64_t i = 0; i < lim; i++)

#ifndef VENOM_RELEASE
#define LogWarning(...) { sprintf(GetDebugMemory()->debugLog.temp_buffer, __VA_ARGS__); PushLogEntry(&GetDebugMemory()->debugLog, LogLevel_WARNING); }
#define LOG_ERROR(...) { sprintf(GetDebugMemory()->debugLog.temp_buffer, __VA_ARGS__); PushLogEntry(&GetDebugMemory()->debugLog, LogLevel_ERROR); }
#define LOG_DEBUG(...) { sprintf(GetDebugMemory()->debugLog.temp_buffer, __VA_ARGS__); PushLogEntry(&GetDebugMemory()->debugLog, LogLevel_DEBUG); }
#else//!VENOM_RELEASE
#define LOG_ERROR(...)
#define LOG_DEBUG(...)
#endif//VENOM_RELEASE

#ifdef VENOM_PROFILER 
#define BEGIN_TIMED_BLOCK(name) BeginPeristantProfileEntry(&GetDebugMemory()->profileData, name)
#define END_TIMED_BLOCK(name) EndPersistantProfileEntry(&GetDebugMemory()->profileData, name)
#else
#define BEGIN_TIMED_BLOCK(name)
#define END_TIMED_BLOCK(name)
#endif

struct SystemInfo {
	int opengl_major_version;
	int opengl_minor_version;
	U64 virtual_memory_size;
	U32 cpu_core_count;
	float screen_width;
	float screen_height;
};

struct InputState {
	B8 isKeyDown[255];
	int keycodes[255];
	U8 keysPressedCount;
	int cursorPosX;
	int cursorPosY;
	int cursorDeltaX;
	int cursorDeltaY;
	B8 isButtonDown[8];

#ifndef VENOM_RELEASE
	bool toggleDebugModePressed;
#endif
};

struct DebugGameplaySettings {
	bool use_debug_camera;
	bool disable_terrain_generation;
};

struct GameState {
	B8 isRunning;
	F32 deltaTime;

#ifndef VENOM_RELEASE
	DebugGameplaySettings debug_settings;
#endif
};

struct DebugRenderSettings {
	bool is_wireframe_enabled;
	bool render_debug_normals;
	bool render_from_directional_light;
	bool draw_shadowmap_depth_texture;
	int draw_shadow_map_index;
};

struct Quad {
	V3 bottom_left;
	V3 top_left;
	V3 top_right;
	V3 bottom_right;
};

struct RenderState {
	Camera camera;
  GBuffer gbuffer;
	Lighting lightingState;

  VenomDrawList drawList;

	GLuint terrain_shader;
	GLuint skydome_shader;
	GLuint material_opaque_shader;
	GLuint material_transparent_shader;

	//Shadow mapping
	GLuint depth_map_shader;
	GLuint depth_map_framebuffer;
	GLuint depth_map_texture;

  CascadedShadowMap csm;
  OmnidirectionalShadowMap osm[4];

	GLuint quadVao;

	MemoryBlock vertexBlock;
	MemoryBlock indexBlock;

#ifndef VENOM_RELEASE
	Camera debugCamera;

	GLuint debug_depth_map_shader;

  VenomDebugRenderSettings debugRenderSettings;
  VenomDebugRenderInfo debugRenderInfo;


	RenderGroup lineDebugGroup;
	RenderGroup solidDebugGroup;
	GLuint debugShader;
	GLuint singleColorShader;
	GLuint debugNormalsShader;

	RenderGroup imguiRenderGroup;
	GLuint imguiRenderGroupShader;
	GLuint imguiFontTexture;

	U32 screenMaxVertexCount;
	ImDrawData *imgui_draw_data;
#endif
};

#ifdef VENOM_HOTLOAD
struct EngineAPI
{
#define EngineAPI(returnType, name, ...) name##Proc name;
EngineAPIList
#undef EngineAPI
#define _(signature, name) signature name;
#include "opengl_procedures.h"
#undef _ 
};
#endif


#include "entity.h"
struct GameMemory {
	SystemInfo systemInfo;
	GameState gameState;
	InputState inputState;
	RenderState renderState;
	GameEntities entities;
	MemoryBlock mainBlock;
	GameAssets assets;
	AudioState audioState;

  void *userdata;

	//TerrainGenerationState terrainGenState;
	//TerrainGenerationParameters terrainGenParams;

	IndexedVertexArray skydomeVertexArray;
	U32 skydomeIndexCount;

#ifdef VENOM_HOTLOAD
	EngineAPI engineAPI;
#endif
#ifndef VENOM_RELEASE
	DebugMemory debug_memory;
#endif
};

#if defined(_WIN32)
#include "platform_windows.h"
#elif defined(__linux__)
#include "platform_linux.h"
#elif defined(__APPLE__)
static_assert(false, "No Apple support");
#endif
