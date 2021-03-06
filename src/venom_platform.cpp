#ifdef __clang__
#pragma clang diagnostic error "-Wreturn-type"
#pragma clang diagnostic warning "-Wall"
#pragma clang diagnostic warning "-Wextra"
#pragma clang diagnostic ignored "-Wformat-security"
#if !defined(VENOM_RELEASE) && !defined(VENOM_STRICT)
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#pragma clang diagnostic ignored "-Wunused-variable"
#endif //VENOM_RELEASE && VENOM_STRICT
#endif//__clang__

#include "venom_platform.h"

#define _(rettype, name, ...) rettype name(__VA_ARGS__);
EngineAPIList
#undef _

#ifndef VENOM_RELEASE
//NOTE(Torin) GetDebugMemory is used to access the DebugLog
//in development builds to keep logging and profiling simple
//NOTE(Torin) These globals are used in debug builds for easy inter program
//introspection and allowing the signal handler to serialize unsaved engine data
//in the event of a engine crash
static GameMemory* _venomEngineData;
GameMemory* GetVenomEngineData() { return _venomEngineData; };
AssetManifest *get_asset_manifest() { return &_venomEngineData->assetManifest;  }
AssetManifest *GetAssetManifest() { return &_venomEngineData->assetManifest; };
#endif//VENOM_RELEASE

//TODO(Torin) Move this somewhere usefull
static int VenomCopyFile(const char *a, const char *b);

#ifdef VENOM_SINGLE_TRANSLATION_UNIT
#include "utility/StringUtils.cpp"
#include "math/math_procedural.cpp"

#include "venom_memory.cpp"
#include "venom_debug.cpp"
#include "venom_render.cpp"


#include "physics/volume.cpp"
#include "physics/collision.cpp"
#include "physics/simulation.cpp"

#include "assets/venom_asset.cpp"
#include "venom_audio.cpp"

#include "utility/serializer.cpp"
#include "utility/profiler.cpp"

#include "Render/Camera.cpp"
#include "Render/PrimitiveRenderer.cpp"
#include "Render/RenderText.cpp"
#include "engine.cpp"

#include "terrain.cpp"
#include "NewTerrain.cpp"
#include "InstancedTerrain.cpp"

#include "Game/CameraMovement.cpp"
#include "Game/CharacterMovement.cpp"
#include "Game/MousePicker.cpp"


#include "Editor/Draw.cpp"
#include "Editor/DrawEditorTabs.cpp"
#include "Editor/Editor.cpp"


#endif//VENOM_SINGLE_TRANSLATION_UNIT

#ifndef VENOM_DEFAULT_SCREEN_WIDTH
#define VENOM_DEFAULT_SCREEN_WIDTH 1280
#endif//VENOM_DEFAULT_SCREEN_WIDTH
#ifndef VENOM_DEFAULT_SCREEN_HEIGHT
#define VENOM_DEFAULT_SCREEN_HEIGHT 720
#endif//VENOM_DEFAULT_SCREEN_HEIGHT

#ifdef VENOM_HOTLOAD
struct VenomModule { void *handle; };
typedef void (*VenomModuleLoadProc)(GameMemory*);
typedef void (*VenomModuleStartProc)(GameMemory*);
typedef void (*VenomModuleUpdateProc)(GameMemory*);
typedef void (*VenomModuleRenderProc)(GameMemory*);
static VenomModuleLoadProc _VenomModuleLoad;
static VenomModuleStartProc _VenomModuleStart;
static VenomModuleUpdateProc _VenomModuleUpdate;
static VenomModuleRenderProc _VenomModuleRender;
static inline void LoadVenomModule(VenomModule* module, const char* path);
static inline void UnloadVenomModule(VenomModule* module);
static inline void ReloadVenomModuleIfModified(VenomModule *module);
#else//!VENOM_HOTLOAD
#include "test_scene.cpp"
#endif//VENOM_HOTLOAD

struct UserConfig {
	U16 screen_width;
	U16 screen_height;
	B8 is_fullscreen;
	U64 memory_size;
};

static inline 
UserConfig GetUserConfig() {
	UserConfig result = {};
	FILE *file = fopen(VENOM_USER_CONFIG_PATH, "rb");
	if (file != NULL) {
		fread(&result, sizeof(UserConfig), 1, file);
		fclose(file);
	}

	else {
		result.screen_width  = VENOM_DEFAULT_SCREEN_WIDTH;
		result.screen_height = VENOM_DEFAULT_SCREEN_HEIGHT;
		result.is_fullscreen = false;
		result.memory_size = MEGABYTES(512);

		file = fopen(VENOM_USER_CONFIG_PATH, "wb");
		if (file == NULL) 
      LogError("Could not write out user config file!");
		fwrite(&result, sizeof(UserConfig), 1, file);
		fclose(file);
	}

	return result;
}

#if 0
#define MODULE_FILE_LIST \
  _(main) \
  _(opengl_render) \

enum ModuleFileList {
  #define _(name) ModuleFile_##name, 
  MODULE_FILE_LIST
  ModuleFileList_COUNT,
  #undef _
};

static const char* ModuleFileNames[] = {
#define _(name) "../src/" #name ".cpp",
  MODULE_FILE_LIST
#undef _
};
#endif

#if 0
static inline 
void PlatformDebugUpdate(GameMemory *memory, VenomModule* module) {


#ifdef _MSC_VER
#define popen _popen
#define pclose _pclose
#endif

#ifdef VENOM_HOTLOAD
	static bool should_compile = true;
  static U64 lastWriteTimes[ModuleFileList_COUNT];
  for(size_t i = 0; i < ModuleFileList_COUNT; i++) {
    U64 currentWriteTime = GetFileLastWriteTime(ModuleFileNames[i]); 
    if(currentWriteTime != lastWriteTimes[i]) {
      lastWriteTimes[i] = currentWriteTime;
      should_compile = true;
      LogWarning("%s was modified.  Recompiling module...", ModuleFileNames[i]);
    }
  }

	if (should_compile) {
		char buffer[1024*256];
		size_t write_index = 0;
		FILE *stream = popen("sh ../src/build_game.sh 2>&1", "r");
		assert(stream != NULL);
		
		while (fgets(&buffer[write_index], ARRAY_COUNT(buffer) - write_index, stream)) {
			write_index += strlen(&buffer[write_index]);
			assert(write_index < ARRAY_COUNT(buffer));
		}

		buffer[write_index] = 0;
    if (buffer[0] != 0) {
      LOG_ERROR(buffer);
    }

		pclose(stream);
		should_compile = false;
		LOG_DEBUG("Game Module Compilation Complete");

    UnloadVenomModule(module);
    LoadVenomModule(module, VENOM_MODULE_FILENAME);
    _VenomModuleLoad(memory);
	}
#endif

}
#endif

static inline 
void PlatformKeyEventHandler(GameMemory *memory, int keycode, int keysym, int isDown, B8 wasDown) {
  InputState* input = &memory->inputState;
  input->isKeyDown[keycode] = isDown;
  auto engine = GetEngine();

  if (wasDown == false && isDown) {
    input->isKeyPressed[keycode] = true;
  } else {
    input->isKeyPressed[keycode] = false;
  }

  if (isDown) {
	  switch(keycode) {
		case KEYCODE_CAPSLOCK: {
		} break;  

    case KEYCODE_TILDA: {
      engine->isConsoleVisible = !engine->isConsoleVisible;
    } break;

    case KEYCODE_F2: {
      memory->editor.isEditorVisible = !memory->editor.isEditorVisible;
    } break;

    case KEYCODE_F3: {
      memory->editor.isSearchWindowOpen = !memory->editor.isSearchWindowOpen;
    } break;

    case KEYCODE_F4: {
      memory->editor.isVisualizerVisible = !memory->editor.isVisualizerVisible;
    }

               

	  } 
  }

  if(memory->keyEventCallback != 0) {
    memory->keyEventCallback(keycode, keysym, isDown);
  }
}

//TODO(Torin) Make this procedure take a availible system memory
//variable so that we can check if there is enough memory availible
//to allocate before attemping to initalize the game
static inline GameMemory* AllocateGameMemory(UserConfig* config) {
  U8* rawMemory = (U8*)MemoryAllocate(config->memory_size);
  memset(rawMemory, 0x00, config->memory_size);
  GameMemory* memory = (GameMemory*)rawMemory;

  //TODO(Torin)Make sure this is aligned on an 8byte boundry
  memory->mainBlock.base = (U8*)(memory + 1);
	memory->mainBlock.size = config->memory_size - sizeof(GameMemory);
	memory->mainBlock.used = 0;
  memory->isRunning = true;
  memory->deltaTime = 1.0f / 60.0f;

  SystemInfo* sys = &memory->systemInfo;  
  sys->screenWidth = config->screen_width;
	sys->screenHeight = config->screen_height;	
	glGetIntegerv(GL_MAJOR_VERSION, &sys->openglMajorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &sys->openglMinorVersion);


#ifdef VENOM_HOTLOAD
#define _(returnType, name, ...) memory->engineAPI.name = name;
  EngineAPIList
#undef _
#define _(signature, name) memory->engineAPI.name = name;
#include "opengl_procedures.h"
#undef _ 
#endif //VENOM_HOTLOAD

    InitalizeEngine();


  //TODO(Torin)An Error log is also needed in release mode this should be rethought
#ifndef VENOM_RELEASE
  _venomEngineData = memory;
#endif

  return memory;
}

//TODO(Torin) Make VENOM_WINDOWS / VENOM_LINUX
//defines rather than the compiler builtins
#if defined(_WIN32)
#define WindowsPlatformMain main
#include "platform/windows.cpp"
#include "platform/concurrency_windows.cpp"
#elif defined(__linux__)
#define LinuxMain main
#include "platform/linux.cpp"
#elif defined(__APPLE__)
static_assert(false, "No apple support yet!");
#elif defined (__EMSCRIPTEN__)
static_assert(false, "No emscripten support yet!")
#endif

#ifndef VENOM_RELEASE
#undef malloc
#undef free
#undef realloc
#define malloc(size) MemoryAllocate(size)
#define free(ptr) MemoryFree(ptr)
#define realloc(ptr, size) MemoryReAllocate(ptr, size)
#include "thirdparty/imgui.cpp"
#include "thirdparty/imgui_draw.cpp"
#include "thirdparty/imgui_demo.cpp"
#undef malloc
#undef free
#undef realloc
#define malloc(size) static_assert(false, "USE CUSTOM ALLOCATORS");
#define free(ptr) static_assert(false, "USE CUSTOM ALLOCATORS");
#define realloc(ptr, size) static_assert(false, "USE CUSTOM ALLOCATORS");
#endif//VENOM_RELEASE