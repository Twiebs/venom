#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <cmath>

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t   S8;
typedef int16_t  S16;
typedef int32_t  S32;
typedef int64_t  S64;
typedef int32_t  B32;
typedef float    F32;
typedef double   F64;
typedef bool B8;

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
#ifdef VENOM_RELEASE
#undef VENOM_HOTLOAD
#undef VENOM_PROFILER
#endif//VENOM_RELEASE

#include "venom_memory.h"
#include "venom_math.h"
#include "venom_audio.h"
#include "venom_render.h"
#include "venom_physics.h"
#include "venom_asset.h"
#include "venom_entity.h"
#ifndef VENOM_RELEASE
#include "venom_serializer.h"
#include "venom_debug.h"
#include "venom_editor.h"
#endif//VENOM_RELEASE

struct GameMemory;

#ifndef VENOM_RELEASE
#define EngineDEBUGAPI \
  _(U64, GetPerformanceCounterTime) \
  _(U64, GetPerformanceCounterFrequency) \
  _(U64, GetFileLastWriteTime, const char *)  \
  _(VenomDebugData*, GetDebugData) \
  _(GameMemory *, GetVenomEngineData)


#else //!VENOM_RELEASE
#define EngineDEBUGAPI
#endif

#define EngineAPIList				              \
  EngineDEBUGAPI

#ifdef VENOM_HOTLOAD
#define _(returnType, name, ...) typedef returnType (*name##Proc)(__VA_ARGS__);
EngineAPIList
#undef _
#endif//VENOM_HOTLOAD

#define fori(count) for(size_t i = 0; i < count; i++)

#ifndef VENOM_RELEASE
#define LogError(...) { sprintf(GetDebugData()->debugLog.temp_buffer, __VA_ARGS__); PushLogEntry(GetDebugData(), LogLevel_ERROR); }
#define LogWarning(...) { sprintf(GetDebugData()->debugLog.temp_buffer, __VA_ARGS__); PushLogEntry(GetDebugData(), LogLevel_WARNING); }
#define LogDebug(...) { sprintf(GetDebugData()->debugLog.temp_buffer, __VA_ARGS__); PushLogEntry(GetDebugData(), LogLevel_DEBUG); }

#define LOG_ERROR(...) { sprintf(GetDebugData()->debugLog.temp_buffer, __VA_ARGS__); PushLogEntry(GetDebugData(), LogLevel_ERROR); }

#define LOG_DEBUG(...) { sprintf(GetDebugData()->debugLog.temp_buffer, __VA_ARGS__); PushLogEntry(GetDebugData(), LogLevel_DEBUG); }
#else//!VENOM_RELEASE
#define LOG_ERROR(...)
#define LOG_DEBUG(...)
#endif//VENOM_RELEASE

//TODO(Torin) Remove this absurd hack
typedef void (*VenomKeyeEventCallbackPROC)(int keycode, int keysym, int keystate);

struct SystemInfo {
	S32 opengl_major_version;
	S32 opengl_minor_version;
	U64 virtual_memory_size;
	U32 cpu_core_count;
	F32 screen_width;
	F32 screen_height;
};

struct InputState {
	B8 isKeyDown[255];
	S32 cursorPosX;
	S32 cursorPosY;
	S32 cursorDeltaX;
	S32 cursorDeltaY;
	B8 isButtonDown[8];
};

//Rename to somthing better?
struct RenderState {
  VenomDrawList drawList;
  GBuffer gbuffer;
  SSAO ssao;
  CascadedShadowMap csm;
  OmnidirectionalShadowMap osm[SHADOW_CASTING_POINT_LIGHT_MAX];
	GLuint quadVao;

#ifndef VENOM_RELEASE
  Camera debugCamera;

  VenomDebugRenderSettings debugRenderSettings;
  VenomDebugRenderFrameInfo debugRenderFrameInfo;
  DebugRenderResources debugRenderResources;

  //IndexedVertexArray debugVertexArray;

  RenderGroup imguiRenderGroup;
	GLuint imguiRenderGroupShader;
	GLuint imguiFontTexture;

	//U32 screenMaxVertexCount;
  //TODO(Torin) Remove this
	ImDrawData *imgui_draw_data;

#endif
};

#ifdef VENOM_HOTLOAD
struct VenomAPI {
#define _(returnType, name, ...) name##Proc name;
EngineAPIList
#undef _ 
#define _(signature, name) signature name;
#include "opengl_procedures.h"
#undef _ 
};
#endif

struct GameMemory {
	SystemInfo systemInfo;
	InputState inputState;
	RenderState renderState;
	AudioState audioState;
  AssetManifest assetManifest;
	MemoryBlock mainBlock;

  B8 isRunning;
	F32 deltaTime;
  void *userdata;

  //TODO(Torin)
  //HACKS to send keydata to the imgui io in the module translation unit
  VenomKeyeEventCallbackPROC keyEventCallback;
#ifdef VENOM_HOTLOAD
	VenomAPI engineAPI;
#endif
#ifndef VENOM_RELEASE
	VenomDebugData debugData;
#endif
};

#if defined(_WIN32)
#include "platform_windows.h"
#elif defined(__linux__)
#include "platform_linux.h"
#elif defined(__APPLE__)
static_assert(false, "No Apple support");
#endif
