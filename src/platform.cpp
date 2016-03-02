#define _CRT_SECURE_NO_WARNINGS 
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

#include "platform.h"

#ifndef VENOM_RELEASE
global_variable DebugMemory* g_debug_memory;
DebugMemory *GetDebugMemory() { return g_debug_memory; }
#endif //NOTE(Torin) GetDebugMemory is used to access the DebugLog
//In error loging functions in development builds


#if 1
#define _(signature, name) static signature name;
#include "opengl_procs.h"
#undef _ 
#endif

#include "offline_asset_tools.h"

//TODO(Torin) Make the single translation unit compiling
//a bit more modular and configurable
#ifdef VENOM_SINGLE_TRANSLATION_UNIT
#ifndef VENOM_RELEASE
#include "engine_debug.cpp"
#include "offline_asset_tools.cpp"
#endif
#include "opengl_glsl.cpp"
#include "opengl_resources.cpp"
#include "venom_asset.cpp"
#include "venom_audio.cpp"
#endif

//TODO(Torin) Consolodate this into a venom_config.h file?
//however its is only used in this one place
#define VENOM_USER_CONFIG_PATH "userdata"

struct UserConfig
{
	U16 screen_width;
	U16 screen_height;
	bool is_fullscreen;
	U64 memory_size;
};

static inline UserConfig GetUserConfig()
{
	UserConfig result = {};
	FILE *file = fopen(VENOM_USER_CONFIG_PATH, "rb");
	if (file != NULL)
	{
		fread(&result, sizeof(UserConfig), 1, file);
		fclose(file);
	}
	else
	{
		//Default UserConfig Settings
		result.screen_width = 1280;
		result.screen_height = 720;
		//TODO(Torin) Remove this 

#if 0
		result.screen_width = 1792;
		result.screen_height = 1008;
#endif

		result.is_fullscreen = false;
		result.memory_size = GIGABYTES(2);

		file = fopen(VENOM_USER_CONFIG_PATH, "wb");
		if (file == NULL) LOG_ERROR("Could not write out user config file!");
		fwrite(&result, sizeof(UserConfig), 1, file);
		fclose(file);
	}

	return result;
}
static inline void platform_independent_tick(GameMemory *memory)
{

#ifdef _MSC_VER
#define popen _popen
#define pclose _pclose
#endif

#ifdef VENOM_HOTLOAD
	static bool should_compile = true;
	static U64 last_write_time = GetFileLastWriteTime("../src/main.cpp");
	U64 current_write_time = GetFileLastWriteTime("../src/main.cpp");

	if (should_compile)
	{
		char buffer[1024];
		size_t write_index = 0;
		FILE *stream = popen("sh ../src/build_game.sh", "r");
		//FILE *stream = popen("echo hello world", "r");
		assert(stream != null);
		
		while (fgets(&buffer[write_index], 1024 - write_index, stream))
		{
			write_index += strlen(&buffer[write_index]);
			assert(write_index < ARRAY_COUNT(buffer));
		}
		buffer[write_index] = 0;
		if (write_index > 0)
		  LOG_ERROR(buffer);
		pclose(stream);
		last_write_time = current_write_time;
		should_compile = false;
		LOG_DEBUG("Game Module Compilation Complete");
	}

	if(last_write_time != current_write_time)
	{
		should_compile = true;
		LOG_DEBUG("Compiling Game Module...");
	}
#endif

	HotloadShaders(&memory->assets);
}

static inline void platform_keyevent_proc(GameMemory *memory, int keycode, int is_down)
{
  
  if (is_down)
  {
	  switch(keycode)
	  {
		case KEYCODE_ESCAPE:
		{
			memory->inputState.toggleDebugModePressed = true;
		} break;  
	  } 
  }
}

static inline void platform_frame_end_proc(GameMemory *memory)
{
	InputState *input = &memory->inputState;
	input->toggleDebugModePressed = false;

}



static inline void InitalizeGameMemory(GameMemory *memory, UserConfig *config)
{
	GameState *gameState = &memory->gameState;
	RenderState *rs = &memory->renderState;
	InputState *input = &memory->inputState;
	SystemInfo *sys = &memory->systemInfo;
	
	memory->mainBlock.base = (U8*)(memory + 1);
	memory->mainBlock.size = config->memory_size - sizeof(GameMemory);
	memory->mainBlock.used = 0;

	*gameState = {};
	gameState->isRunning = true;
	gameState->deltaTime = 1.0f / 60.0f;

	*input = {};

#ifdef VENOM_HOTLOAD
	{
		EngineAPI *API = &memory->engineAPI;

#define EngineAPI(returnType, name, ...) API->name = name;
		EngineAPIList
#undef EngineAPI
#define _(signature, name) API->name = name;
#include "opengl_procs.h"
#undef _ 
	}
#endif

	sys->screen_width = config->screen_width;
	sys->screen_height = config->screen_height;	
	glGetIntegerv(GL_MAJOR_VERSION, &sys->opengl_major_version);
	glGetIntegerv(GL_MINOR_VERSION, &sys->opengl_minor_version);


#ifndef VENOM_RELEASE
	g_debug_memory = &memory->debug_memory;
#endif

	InitalizeGameAssets(&memory->assets);
}
//TODO(Torin) Consider moving the hotload configuration defines
//Out into unified location for engine configuration information
//
//TODO(Torin)Rename the module procedure to start with PROC instead of their name
#ifdef VENOM_HOTLOAD
#define HOTLOAD_TRIGGER_DLL "hotload_trigger.dll" 
#define HOTLOAD_OUTPUT_DLL "game.dll"
typedef void (*OnModuleLoadProc)(GameMemory*);
typedef void (*GameStartupProc)(GameMemory*);
typedef void (*GameUpdateProc)(GameMemory*);
typedef void (*GameRenderProc)(GameMemory*);
global_variable OnModuleLoadProc OnModuleLoad;
global_variable GameStartupProc GameStartup;
global_variable GameUpdateProc GameUpdate;
global_variable GameRenderProc GameRender;
#define LoadGameCode() InternalLoadGameCode()
#else
//TODO(Torin) main.cpp is hardcoded in many locations consider making it modular
#include VENOM_HOTLOAD_MODULE_FILE 
#define LoadGameCode()
#endif

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
