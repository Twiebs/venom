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

#define global_variable static
#define null nullptr

#include "venom_config.h"

//Configuration Post-Processing
#ifdef VENOM_RELEASE
#undef VENOM_HOTLOAD
#undef VENOM_PROFILER
#endif

#define KILOBYTES(x) ((x) * 1024LL)
#define MEGABYTES(x) (KILOBYTES(x) * 1024LL)
#define GIGABYTES(x) (MEGABYTES(x) * 1024LL)

#define ARRAY_COUNT(array) (sizeof(array) / sizeof(*array))
#define static_strlen(string) ((sizeof(string)) - 1)

#ifndef _DEBUG
#define strict_assert(expr)
#else
#define strict_assert(expr) assert(expr)
#endif


#define MEMORY_BLOCK_MAX_CHILDREN 8
struct MemoryBlock
{
	U8* base;
	size_t size;
	size_t used;

#ifndef VENOM_RELEASE
	const char *name;
	MemoryBlock *children[MEMORY_BLOCK_MAX_CHILDREN];
	U32 childCount;
#endif
};

#define PushStruct(structType, memblock) (structType*)PushSize(sizeof(structType), memblock)
#define PushArray(ElementType, ElementCount, Elements,Arena) memcpy(PushSize(sizeof(ElementType) * ElementCount, Arena), Elements, sizeof(ElementType) * ElementCount)
#define ReserveArray(elementType, elementCount, memoryBlock) ((elementType *)PushSize(sizeof(elementType) * elementCount, memoryBlock))
#define AquireArrayMemory(elements, count, memory) elements = ((decltype(elements)) PushSize(sizeof(*elements) * count, memory))
#define InitSubBlock(name, block, size, parent) _SetBlockName(block, name); _SubBlock(block, size, parent)

#include "venom_math.h"
#include "venom_render.h"
#include "venom_audio.h"
#include "venom_physics.h"
#include "venom_asset.h"

#include "terrain.h"
#include "imgui.h"

#ifndef VENOM_RELEASE
#include "venom_debug.h"

struct DebugMemory 
{
	DebugLog debugLog;
	ProfileData profileData;
};

#define EngineDEBUGAPI \
	EngineAPI(U64, GetPerformanceCounter) \
	EngineAPI(U64, GetPerformanceFrequency) \
	EngineAPI(U64, GetFileLastWriteTime, const char *)  \
	EngineAPI(DebugMemory*, GetDebugMemory) \

	//EngineAPI(ModelData, ImportExternalModelData, const char*, MemoryBlock)

#define fori(lim) for(int64_t i = 0; i < lim; i++)

#define LogWarning(...) { sprintf(GetDebugMemory()->debugLog.temp_buffer, __VA_ARGS__); PushLogEntry(&GetDebugMemory()->debugLog, LogLevel_WARNING); }

#define LOG_ERROR(...) { sprintf(GetDebugMemory()->debugLog.temp_buffer, __VA_ARGS__); PushLogEntry(&GetDebugMemory()->debugLog, LogLevel_ERROR); }
#define LOG_DEBUG(...) { sprintf(GetDebugMemory()->debugLog.temp_buffer, __VA_ARGS__); PushLogEntry(&GetDebugMemory()->debugLog, LogLevel_DEBUG); }

#ifdef VENOM_PROFILER 
#define BEGIN_TIMED_BLOCK(name) BeginPeristantProfileEntry(&GetDebugMemory()->profileData, name)
#define END_TIMED_BLOCK(name) EndPersistantProfileEntry(&GetDebugMemory()->profileData, name)
#else
#define BEGIN_TIMED_BLOCK(name)
#define END_TIMED_BLOCK(name)
#endif

#else
#define LOG_ERROR(...)
#define LOG_DEBUG(...)
#define EngineDEBUGAPI
#endif

#define EngineAPIList				              \
	EngineAPI(GLuint, GetShaderProgram, DEBUGShaderID, GameAssets*) \
	EngineAPI(const ModelDrawable&, GetModelDrawable, DEBUGModelID, GameAssets*)\
	EngineAPI(const MaterialDrawable&, GetMaterial, U32, GameAssets*)\
	EngineDEBUGAPI

#ifdef VENOM_HOTLOAD
#define EngineAPI(returnType, name, ...) typedef returnType (*name##Proc)(__VA_ARGS__);
EngineAPIList
#undef EngineAPI
#endif

#ifdef VENOM_ENGINE 
#define EngineAPI(returnType, name, ...) returnType name (__VA_ARGS__);
EngineAPIList
#undef EngineAPI
#endif


inline U8 *PushSize(size_t size, MemoryBlock *block) {
	assert(block->used + size <= block->size);
	U8 *result = block->base + block->used;
	block->used += size;
	return result;
}

inline void _SubBlock(MemoryBlock *child, size_t size, MemoryBlock *parent) {	
	child->size = size;
	child->base = PushSize(size, parent);
	child->used = 0;
#ifndef VENOM_RELEASE
	assert(parent->childCount < MEMORY_BLOCK_MAX_CHILDREN);
	parent->children[parent->childCount++] = child;
#endif
}

inline void _SetBlockName(MemoryBlock *block, const char *name) {
	block->name = name;
}

struct SystemInfo {
	int opengl_major_version;
	int opengl_minor_version;
	U64 virtual_memory_size;
	U32 cpu_core_count;
	float screen_width;
	float screen_height;
};

struct DebugControls {
	bool toggle_debug_mode;
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

	TerrainGenerationState terrainGenState;
	TerrainGenerationParameters terrainGenParams;

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
