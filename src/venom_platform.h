#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include <cmath>
#include <thread>
#include <mutex>

typedef uint8_t   U8;
typedef uint16_t  U16;
typedef uint32_t  U32;
typedef uint64_t  U64;
typedef int8_t    S8;
typedef int16_t   S16;
typedef int32_t   S32;
typedef int64_t   S64;
typedef int32_t   B32;
typedef float     F32;
typedef double    F64;
typedef bool      B8;

static const F32 F32_MIN = FLT_MIN;
static const F32 F32_MAX = FLT_MAX;

static const U8 INVALID_U8    = 0xFF;
static const U16 INVALID_U16  = 0xFFFF;
static const U32 INVALID_U32  = 0xFFFFFFFF;
static const U64 INVALID_U64  = 0xFFFFFFFFFFFFFFFF;

#define KILOBYTES(x) ((x) * 1024LL)
#define MEGABYTES(x) (KILOBYTES(x) * 1024LL)
#define GIGABYTES(x) (MEGABYTES(x) * 1024LL)
#define ARRAY_COUNT(array) (sizeof(array) / sizeof(*array))
#define STATIC_STRLEN(string) ((sizeof(string)) - 1)
#define static_strlen(string) ((sizeof(string)) - 1)
#define strict_assert(expr) assert(expr)
#define StrictAssert(expr) assert(expr)
#define Assert(expr) assert(expr)

#if defined(_WIN32)
#define VENOM_PLATFORM_WINDOWS
#elif defined(__linux__)
#define VENOM_PLATFORM_LINUX
#elif defined(__APPLE__)
#define VENOM_PLATFORM_MAC
#endif //Platform #ifdefs

#include "venom_config.h"
#ifdef VENOM_RELEASE
#undef VENOM_HOTLOAD
#undef VENOM_PROFILER
#endif//VENOM_RELEASE

#define malloc(size) static_assert(false, "DONT USE MALLOC");
#define calloc(size, count) static_assert(false, "DONT USE CALLOC");
#define realloc(ptr, size) static_assert(false, "DOUNT USE REALLOC");
#define free(size, count) static_assert(false, "DONT USE FREE");
#define strdup(str) static_assert(false, "Use CStringDuplicate");

#ifndef VENOM_TRACK_MEMORY
#define MemoryAllocate(size) MemoryAllocateDefault(size)
#define MemoryReAllocate(ptr, size) MemoryReAllocateDefault(ptr, size)
#define MemoryFree(ptr) MemoryFreeDefault(ptr)
inline U8 *MemoryAllocateDefault(size_t size);
inline U8 *MemoryReAllocateDefault(void *ptr, size_t size);
inline void MemoryFreeDefault(void *ptr);
#else//VENOM_TRACK_MEMORY
#define MemoryAllocate(size) MemoryAllocateTracked(size, __LINE__, __FILE__, __FUNCTION__)
#define MemoryReAllocate(ptr, size) MemoryReAllocateTracked(ptr, size, __LINE__, __FILE__, __FUNCTION__)
#define MemoryFree(ptr) MemoryFreeTracked(ptr, __LINE__, __FILE__, __FUNCTION__)
inline U8 *MemoryAllocateTracked(size_t size, size_t lineNumber, const char *file, const char *function);
inline U8 *MemoryReAllocateTracked(void *ptr, size_t size, size_t lineNumber, const char *file, const char *function);
inline void MemoryFreeTracked(void *ptr, size_t lineNumber, const char *file, const char *function);
#endif//VENOM_TRACK_MEMORY

struct SystemTime {
  U16 day;
  U16 hour;
  U16 minute;
  U16 second;
};

struct Engine;
Engine *GetEngine();

#include "platform/concurrency.h"

#include "venom_memory.h"
#include "math/venom_math.h"
#include "assets/venom_asset.h"
#include "venom_audio.h"
#include "animation.h"

#include "Render/Camera.h"
#include "venom_render.h"

#include "physics/volume.h"
#include "venom_entity.h"

#include "physics/collision.h"
#include "physics/simulation.h"

#include "assets/asset_data.h"
#include "terrain.h"

#include "NewTerrain.h"

#include "debug_renderer.h"

#include "Render/PrimitiveRenderer.h"
#include "Render/RenderText.h"


#ifndef VENOM_RELEASE
#include "utility/profiler.h"
#include "utility/serializer.h"
#include "venom_debug.h"
#include "Editor/Editor.h"
#endif//VENOM_RELEASE

#ifndef VENOM_RELEASE
#define DebugEnableIf(expr) if (expr)
#define DebugDisableIf(expr) if(!expr)
#define DEBUG_DisableIf(expr) if(!expr)
#else//VENOM_RELEASE
#define DebugDisableIf(expr) if (1)
#define DebugEnableIf(expr) if (0)
#endif//VENOM_RELEASE

struct GameMemory;

#ifndef VENOM_RELEASE
#define EngineDEBUGAPI \
  _(U64, GetPerformanceCounterTime) \
  _(U64, GetPerformanceCounterFrequency) \
  _(U64, GetFileLastWriteTime, const char *)  \
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
#define LogError(...) CreateLogEntry(LogLevel_ERROR, __VA_ARGS__)
#define LogWarning(...) CreateLogEntry(LogLevel_WARNING, __VA_ARGS__)
#define LogDebug(...) CreateLogEntry(LogLevel_DEBUG, __VA_ARGS__)
#else//!VENOM_RELEASE
#define LogError(...)
#define LogWarning(...)
#define LogDebug(...)
#endif//VENOM_RELEASE

//TODO(Torin) Remove this absurd hack
typedef void (*VenomKeyeEventCallbackPROC)(int keycode, int keysym, int keystate);

struct SystemInfo {
	S32 openglMajorVersion;
	S32 openglMinorVersion;
	U64 virtualMemorySize;
	U32 cpuCoreCount;
	F32 screenWidth;
	F32 screenHeight;
};

struct InputState {
	B8 isKeyDown[255];
  B8 isKeyPressed[255];
  B8 isButtonDown[8];
  B8 isButtonPressed[8];
	S32 cursorPosX;
	S32 cursorPosY;
	S32 cursorDeltaX;
	S32 cursorDeltaY;
};

struct Worker {
  U32 workerID;
  std::thread thread;
  StaticSizedStack stackMemory;
};

enum TaskType {
  TaskType_LoadModel,
};

struct Task {
  TaskType type;
  union {
    struct {
      U32 slotID;
    };
  };
};

struct MemoryAllocationEntry {
  const char *filename;
  const char *procedureName;
  size_t lineNumber;
  uintptr_t memoryAddress;
  size_t allocationSize;
};

struct Engine {
  B8 isRunning;
  B8 isPaused;
  B8 isEditorInFocus;

  PhysicsSimulation physicsSimulation;
  EntityContainer entityContainer;
  Terrain terrain;

  //Venom Task system
  Worker *workers;
  U32 workerCount;
  std::mutex workLock;
  DynamicArray<Task> tasksToExecute;
  DynamicArray<Task> tasksToFinalize;

#ifndef VENOM_RELEASE
  U32 unseenErrorCount;
  U32 unseenWarningCount;
  U32 unseenInfoCount;
  DebugLog debugLog;
  ProfileData profileData;
  B8 isConsoleVisible;

  //Memory usage stastitics
  size_t currentMemoryAllocated;
  SpinLock memoryAllocationLock;
  MemoryAllocationEntry memoryAllocations[1024];
  size_t memoryAllocationCount;
#endif//VENOM_RELEASE
};

#include "engine.h"


//Rename to somthing better?
struct RenderState {
  VenomDrawList drawList;
  GBuffer gbuffer;
  SSAO ssao;
  CascadedShadowMap csm;
  OmnidirectionalShadowMap osm[SHADOW_CASTING_POINT_LIGHT_MAX];
	GLuint quadVao;

  IndexedVertexArray skydomeIVA;

#ifndef VENOM_RELEASE
  VenomDebugRenderSettings debugRenderSettings;
  VenomDebugRenderFrameInfo debugRenderFrameInfo;

  //IndexedVertexArray debugVertexArray;

  RenderGroup imguiRenderGroup;
	GLuint imguiRenderGroupShader;
	GLuint imguiFontTexture;

	//U32 screenMaxVertexCount;
  //TODO(Torin) Remove this
	ImDrawData *imgui_draw_data;

  Terrain *newTerrain;
  TerrainGenerationState *terrain;
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
  EditorData editor;


  B8 isRunning;
	F32 deltaTime;
  void *userdata;

  //TODO(Torin)
  //HACKS to send keydata to the imgui io in the module translation unit
  VenomKeyeEventCallbackPROC keyEventCallback;
#ifdef VENOM_HOTLOAD
	VenomAPI engineAPI;
#endif
};

inline Engine *GetEngine();
inline U32 GetThreadID();

void ScheduleTask(Task task);


SystemTime GetSystemTime();


#if defined(VENOM_PLATFORM_WINDOWS)
#include "platform/windows.h"
#elif defined(VENOM_PLATFORM_LINUX)
#include "platform_linux.h"
#elif defined(VENOM_PLATFORM_MAC)
static_assert(false, "No Mac support");
#endif
