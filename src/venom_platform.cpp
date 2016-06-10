#pragma clang diagnostic error "-Wreturn-type" //FUCK YOU C++
#pragma clang diagnostic warning "-Wall"
#pragma clang diagnostic warning "-Wextra"
#pragma clang diagnostic ignored "-Wformat-security"

#if !defined(VENOM_RELEASE) && !defined(VENOM_STRICT)
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#pragma clang diagnostic ignored "-Wunused-variable"
#endif

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
static VenomDebugData* _debugData;
VenomDebugData *GetDebugData() { return _debugData; }
GameMemory* GetVenomEngineData() { return _venomEngineData; };
#endif//VENOM_RELEASE

#ifdef VENOM_SINGLE_TRANSLATION_UNIT
//NOTE(Torin)These files are the core functionality
//of venom and may include additional .cpp files however;
//that is the extend that includes are allowed to go
#include "venom_debug.cpp"
#include "venom_render.cpp"
#include "venom_physics.cpp"
#include "venom_asset.cpp"
#include "venom_audio.cpp"
#include "venom_serializer.cpp"
#endif//VENOM_SINGLE_TRANSLATION_UNIT

//NOTE(Torin) Set default configuration macros if 
//they were left unspecified 
#ifndef VENOM_DEFAULT_SCREEN_WIDTH
#define VENOM_DEFAULT_SCREEN_WIDTH 1280
#endif//VENOM_DEFAULT_SCREEN_WIDTH
#ifndef VENOM_DEFAULT_SCREEN_HEIGHT
#define VENOM_DEFAULT_SCREEN_HEIGHT 720
#endif//VENOM_DEFAULT_SCREEN_HEIGHT

#ifdef VENOM_HOTLOAD
typedef void (*VenomModuleLoadProc)(GameMemory*);
typedef void (*VenomModuleStartProc)(GameMemory*);
typedef void (*VenomModuleUpdateProc)(GameMemory*);
typedef void (*VenomModuleRenderProc)(GameMemory*);
static VenomModuleLoadProc _VenomModuleLoad;
static VenomModuleStartProc _VenomModuleStart;
static VenomModuleUpdateProc _VenomModuleUpdate;
static VenomModuleRenderProc _VenomModuleRender;
#else//!VENOM_HOTLOAD
#include VENOM_MODULE_SOURCE_FILENAME
#endif//VENOM_HOTLOAD

//========================================================================================
//========================================================================================
//========================================================================================

//NOTE(Torin) Represented in a file stored on the users device 
struct UserConfig {
	U16 screen_width;
	U16 screen_height;
	B8 is_fullscreen;
	U64 memory_size;
};

struct VenomModule {
  void *handle;
};

static inline void LoadVenomModule(VenomModule* module, const char* path);
static inline void UnloadVenomModule(VenomModule* module);

static inline
void BeginProfileEntryHook(const char* name){
  VenomDebugData* debugData = GetDebugData();
  __BeginProfileEntry(&debugData->profileData, name);
}

static inline 
void EndProfileEntryHook(const char* name){
  VenomDebugData* debugData = GetDebugData();
  __EndProfileEntry(&debugData->profileData, name);
}

#ifndef VENOM_DISABLE_PROFILER
#define BeginProfileEntry(name) BeginProfileEntryHook(name) 
#define EndProfileEntry(name) EndProfileEntryHook(name) 
#else//VENOM_DISABLE_PROFILER
#define BeginProfileEntry(name)
#define EndProfileEntry(name)
#endif//VENOM_DISABLE_PROFILER

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
		result.memory_size = GIGABYTES(2);

		file = fopen(VENOM_USER_CONFIG_PATH, "wb");
		if (file == NULL) 
      LOG_ERROR("Could not write out user config file!");
		fwrite(&result, sizeof(UserConfig), 1, file);
		fclose(file);
	}

	return result;
}

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

static inline 
void PlatformKeyEventHandler(GameMemory *memory, int keycode, int keysym, int isDown) {
  InputState* input = &memory->inputState;
  input->isKeyDown[keycode] = isDown;

  if (isDown) {
	  switch(keycode) {
		case KEYCODE_CAPSLOCK: {
		} break;  

    case KEYCODE_TILDA: {
      memory->debugData.triggerToggleConsole = true;
    } break;
	  } 
  }

  if(memory->keyEventCallback != 0) {
    memory->keyEventCallback(keycode, keysym, isDown);
  }
}

static inline void platform_frame_end_proc(GameMemory *memory){
	InputState *input = &memory->inputState;
}

static void
VenomSignalHandler(int signal){
  //WriteAssetManifestFile("assets.manifest", &GetVenomEngineData()->assetManifest);
}

//TODO(Torin) Make this procedure take a availible system memory
//variable so that we can check if there is enough memory availible
//to allocate before attemping to initalize the game
static inline
GameMemory* AllocateGameMemory(UserConfig* config) {
  U8* rawMemory = (U8*)calloc(1, config->memory_size);
  GameMemory* memory = (GameMemory*)rawMemory;

  //TODO(Torin)Make sure this is aligned on an 8byte boundry
  memory->mainBlock.base = (U8*)(memory + 1);
	memory->mainBlock.size = config->memory_size - sizeof(GameMemory);
	memory->mainBlock.used = 0;
  memory->isRunning = true;
  memory->deltaTime = 1.0f / 60.0f;

  //TODO(Torin) Move out these opengl specific routines
  SystemInfo* sys = &memory->systemInfo;  
  sys->screen_width = config->screen_width;
	sys->screen_height = config->screen_height;	
	glGetIntegerv(GL_MAJOR_VERSION, &sys->opengl_major_version);
	glGetIntegerv(GL_MINOR_VERSION, &sys->opengl_minor_version);

#ifdef VENOM_HOTLOAD
#define _(returnType, name, ...) memory->engineAPI.name = name;
  EngineAPIList
#undef _
#define _(signature, name) memory->engineAPI.name = name;
#include "opengl_procedures.h"
#undef _ 
#endif //VENOM_HOTLOAD

  //TODO(Torin)An Error log is also needed in release mode this should be rethought
#ifndef VENOM_RELEASE
	_debugData = &memory->debugData;
  _venomEngineData = memory;
#endif

  return memory;
}

//TODO(Torin) Make VENOM_WINDOWS / VENOM_LINUX
//defines rather than the compiler builtins
#if defined(_WIN32)
#define WindowsPlatformMain main
#include "platform_windows.cpp"
#elif defined(__linux__)
#define LinuxMain main
#include "platform_linux.cpp"
#elif defined(__APPLE__)
static_assert(false, "No apple support yet!");
#elif defined (__EMSCRIPTEN__)
static_assert(false, "No emscripten support yet!")
#endif
