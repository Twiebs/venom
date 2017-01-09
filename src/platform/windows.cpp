namespace Win32 {
#define WIN32_LEAN_AND_MEAN
#undef malloc
#undef free
#undef realloc
#define malloc(size) MemoryAllocate(size)
#define free(ptr) MemoryFree(ptr)
#define realloc(ptr, size) MemoryReAllocate(ptr, size)
#include <Windows.h>
#include <windowsx.h>
#include <mmeapi.h>
#include <dsound.h>
#undef malloc
#undef free
#undef realloc
#define malloc(size) static_assert(false, "USE CUSTOM ALLOCATORS");
#define free(ptr) static_assert(false, "USE CUSTOM ALLOCATORS");
#define realloc(ptr, size) static_assert(false, "USE CUSTOM ALLOCATORS");
};

U64 GetFileLastWriteTime(const char *filename) { 
  using namespace Win32;
	U64 result = 0;
	WIN32_FIND_DATA findData;
	HANDLE handle = FindFirstFile(filename, &findData);
	if (handle != INVALID_HANDLE_VALUE)
	{
		result = findData.ftLastWriteTime.dwLowDateTime;
		result += ((U64)findData.ftLastWriteTime.dwHighDateTime << 32);
		FindClose(handle);
	}
	return result;
}

U64 GetPerformanceCounterTime() {
  using namespace Win32;
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  U64 result = (U64)counter.QuadPart;
  return result;
}

U64 GetPerformanceCounterFrequency() {
  using namespace Win32;
  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency);
  U64 result = (U64)frequency.QuadPart;
  return result;
}

SystemTime GetSystemTime() {
  Win32::SYSTEMTIME systemtime;
  Win32::GetSystemTime(&systemtime);
  SystemTime result = {};
  result.day = systemtime.wDay;
  result.hour = systemtime.wHour;
  result.minute = systemtime.wMinute;
  result.second = systemtime.wSecond;
  return result;
}

#ifdef VENOM_VULKAN
#include "vulkan/vulkan.h"
static inline
void vulkan_initalize()
{
	HMODULE module = LoadLibraryA("vulkan-1.dll");
	if (module)
	{
		VkApplicationInfo appInfo = {};
		appInfo.apiVersion = 1;
		appInfo.applicationVersion = 1;
		appInfo.engineVersion = 1;
		appInfo.pApplicationName = "venom_test";
		appInfo.pEngineName = "venom";
	}
}
#endif



#ifdef VENOM_HOTLOAD
struct Win32GameCode
{
	HMODULE module;
	FILETIME lastWriteTime;
};

static Win32GameCode g_code;

static inline
FILETIME GetLastWriteTime(const char *filename)
{
	FILETIME lastWriteTime = {};
	WIN32_FIND_DATA find_data;
	HANDLE handle = FindFirstFile(filename, &find_data);
	if (handle != INVALID_HANDLE_VALUE)
	{
		lastWriteTime = find_data.ftLastWriteTime;
		FindClose(handle);
	}	
	return lastWriteTime;
}

void InternalLoadGameCode()
{
	Win32GameCode *code = &g_code;
	CopyFile(HOTLOAD_TRIGGER_DLL, HOTLOAD_OUTPUT_DLL, FALSE);
	code->module = LoadLibraryA(HOTLOAD_OUTPUT_DLL);
	if (code->module)
	{
		OnModuleLoad = (OnModuleLoadProc)GetProcAddress(code->module, "OnModuleLoad");
		GameStartup = (GameStartupProc)GetProcAddress(code->module, "GameStartup");
		GameUpdate = (GameUpdateProc)GetProcAddress(code->module, "GameUpdate");
		GameRender = (GameRenderProc)GetProcAddress(code->module, "GameRender");
		bool isValid = OnModuleLoad && GameStartup && GameUpdate && GameRender;
		if (!isValid)
		{
			LOG_ERROR("Could not load GameFunctions!");
		}
	}
	else
	{
		U32 error = GetLastError();
		LOG_ERROR("Could not load %s: error code %d", HOTLOAD_OUTPUT_DLL, error);
	}
	g_code.lastWriteTime = GetLastWriteTime(HOTLOAD_TRIGGER_DLL);
}
#endif

#if 1
using namespace Win32;
static LRESULT CALLBACK Win32WindowCallback(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = DefWindowProc(window, message, wparam, lparam);
	return result;
}
#endif

U64 GetPerformanceCounter()
{
  using namespace Win32;
	LARGE_INTEGER counterValue;
	QueryPerformanceCounter(&counterValue);
	return counterValue.QuadPart;
}

U64 GetPerformanceFrequency()
{
  using namespace Win32;
	LARGE_INTEGER counterFrequency;
	QueryPerformanceFrequency(&counterFrequency);
	return counterFrequency.QuadPart;
}

int WindowsPlatformMain() {
  using namespace Win32;
	//NOTE(Torin) This will be read in from disk
	//If the serialized file does not exist then we
	//know that this is the first run of the game!
	UserConfig config = GetUserConfig();

	HINSTANCE hInstance = GetModuleHandle(NULL);
	RECT clientViewportArea;
	clientViewportArea.left = 0;
	clientViewportArea.top = 0;
	clientViewportArea.right = (LONG)config.screen_width;
	clientViewportArea.bottom = (LONG)config.screen_height;
	AdjustWindowRect(&clientViewportArea, WS_BORDER, false);

	WNDCLASS window_class = {};
	window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	window_class.lpfnWndProc = Win32WindowCallback;
	window_class.hInstance = hInstance;
	window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	window_class.lpszClassName = "VenomWindowClass";
	if (!RegisterClass(&window_class)) {
    LogError("Failed to create window class");
		PostQuitMessage(1);
	}
	
	HWND window = CreateWindow("VenomWindowClass", "VenomWindowClass",
		WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT, 
		config.screen_width, config.screen_height, 
		NULL, NULL, 
		hInstance, 
		NULL
	);

	if (window == NULL)
	{
		PostQuitMessage(1);
	}


	// @Audio @Sound

	static const int secondaryBufferSize = AUDIO_SAMPLES_PER_SECOND * AUDIO_BYTES_PER_SAMPLE;

	LPDIRECTSOUND directSound;
	LPDIRECTSOUNDBUFFER primaryBuffer;
	LPDIRECTSOUNDBUFFER secondaryBuffer;
	WAVEFORMATEX waveFormat = {};

	{ //Initialize @DirectSound @DSound
		HMODULE directSoundModule = LoadLibraryA("dsound.dll");
		if (directSoundModule == 0) LogError("Failed to load direct sound dll");

		typedef HRESULT WINAPI PROCDirectSoundCreate(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter);
		PROCDirectSoundCreate *DirectSoundCreate = (PROCDirectSoundCreate*)GetProcAddress(directSoundModule, "DirectSoundCreate");
		if (DirectSoundCreate == 0) 
      LogError("Failed to load DirectSoundCreate");
		if (!SUCCEEDED(DirectSoundCreate(0, &directSound, 0)))
      LogError("DirectSoundCreate Failed!");
		if (!SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
      LogError("Failed to set directSound cooperative level!");

		//Create @WaveFormat
		waveFormat.wFormatTag = WAVE_FORMAT_PCM;
		waveFormat.nChannels = 2;
		waveFormat.nSamplesPerSec = AUDIO_SAMPLES_PER_SECOND;
		waveFormat.wBitsPerSample = 16;
		waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
		waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
		waveFormat.cbSize = 0;

		//TODO(Torin) Check the flags on the directsound buffers to make sure they are 
		//actualy what you expect them to be

		//Initialize @PrimarySoundBuffer
		DSBUFFERDESC primaryBufferDescription = {};
		primaryBufferDescription.dwSize = sizeof(primaryBufferDescription);
		primaryBufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
		if (!SUCCEEDED(directSound->CreateSoundBuffer(&primaryBufferDescription, &primaryBuffer, 0)))
      LogError("Failed to Create Primary Sound Buffer");
		if (!SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
      LogError("Failed to set Primary buffer wave format!");

		//Initalize @SecondarySoundBuffer
		DSBUFFERDESC secondaryBufferDescription = {};
		secondaryBufferDescription.dwSize = sizeof(secondaryBufferDescription);
		secondaryBufferDescription.dwFlags = 0;
		secondaryBufferDescription.dwBufferBytes = secondaryBufferSize;
		secondaryBufferDescription.lpwfxFormat = &waveFormat;
    if (!SUCCEEDED(directSound->CreateSoundBuffer(&secondaryBufferDescription, &secondaryBuffer, 0)))
      LogError("Failed to create secondary sound buffer");
		secondaryBuffer->Play(0, 0, DSBPLAY_LOOPING); 
	} //DirectSound

	//@OpenGLContext
	PIXELFORMATDESCRIPTOR pfd = {};
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	HDC hdc = GetDC(window);
	int pixel_format = ChoosePixelFormat(hdc, &pfd);

	if (pixel_format == 0) return 1;
	if (!SetPixelFormat(hdc, pixel_format, &pfd)) return 1;

	HGLRC temp_context = wglCreateContext(hdc);
	wglMakeCurrent(hdc, temp_context);

	HGLRC render_context;
	{   //Intitalize OpenGL 3.3 Context
		typedef HGLRC(WINAPI *PROCwglCreateContextAttribsARB)(HDC, HGLRC, const int*);
		typedef BOOL(WINAPI *PROCwglChoosePixelFormatARB)(HDC hdc, const int* piAttribIList,
			const FLOAT *pfAttribFList, UINT nMaxFormats, int* piFormats, UINT *numFormats);

		PROCwglCreateContextAttribsARB wglCreateContextAttribsARB;
		PROCwglChoosePixelFormatARB wglChosePixelFormatARB;
		wglCreateContextAttribsARB = (PROCwglCreateContextAttribsARB)
			wglGetProcAddress("wglCreateContextAttribsARB");
		wglChosePixelFormatARB = (PROCwglChoosePixelFormatARB)
			wglGetProcAddress("wglChoosePixelFormatARB");

		static const U32 WGL_DRAW_TO_WINDOW_ARB = 0x2001;
		static const U32 WGL_SUPPORT_OPENGL_ARB = 0x201;
		static const U32 WGL_DOUBLE_BUFFER_ARB = 0x2011;
		static const U32 WGL_PIXEL_TYPE_ARB = 0x2013;
		static const U32 WGL_COLOR_BITS_ARB = 0x2014;
		static const U32 WGL_DEPTH_BITS_ARB = 0x2022;
		static const U32 WGL_STENCIL_BITS_ARB = 0x2023;
		static const U32 WGL_TYPE_RGBA_ARB = 0x202B;

		const int pixel_format_attribs[] = {
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB, 32,
			WGL_DEPTH_BITS_ARB, 32,
			WGL_STENCIL_BITS_ARB, 8,
			0,
		};

		int pixel_format;
		UINT num_formats;
		wglChosePixelFormatARB(hdc, pixel_format_attribs, NULL, 1, &pixel_format, &num_formats);

		static const U32 WGL_CONTEXT_MAJOR_VERSION_ARB = 0x2091;
		static const U32 WGL_CONTEXT_MINOR_VERSION_ARB = 0x2092;
		static const U32 WGL_CONTEXT_LAYER_PLANE_ARB = 0x2093;
		static const U32 WGL_CONTEXT_FLAGS_ARB = 0x2094;
		static const U32 WGL_CONTEXT_PROFILE_MASK_ARB = 0x9126;

		static const U32 WGL_CONTEXT_DEBUG_BIT_ARB = 0x0001;
		static const U32 WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB = 0x0002;

		static const U32 WGL_CONTEXT_CORE_PROFILE_BIT_ARB = 0x00000001;


		int flags = 0, mask = 0;
#ifndef VENOM_RELEASE
		flags |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif
		flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
		mask |= WGL_CONTEXT_CORE_PROFILE_BIT_ARB;

		const int attributes[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 3,
			WGL_CONTEXT_FLAGS_ARB, flags,
			WGL_CONTEXT_PROFILE_MASK_ARB, mask,
			NULL
		};

		render_context = wglCreateContextAttribsARB(hdc, NULL, attributes);
		if (render_context == nullptr) {
			//auto error = glGetError();
			return 1;
		}
	}

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(temp_context);
	wglMakeCurrent(hdc, render_context);

	//Set the inital @GameState
	HMODULE libgl = LoadLibraryA("opengl32.dll");
#define _(signature, name) name = (signature)wglGetProcAddress(#name); \
	if (name == nullptr) name = (signature)GetProcAddress(libgl, #name);
#include "opengl_procedures.h"
#undef _
	FreeLibrary(libgl);


	 //Initalize @Memory
	MEMORYSTATUSEX memoryStatus;
	memoryStatus.dwLength = sizeof(memoryStatus);
	GlobalMemoryStatusEx(&memoryStatus);	
	if (memoryStatus.ullAvailVirtual < config.memory_size)
	{
    LogError("Available system memory(%llu) is less than the configured memory cache size(%llu)",
			memoryStatus.ullAvailVirtual, config.memory_size);
		MessageBox(window, "Not enough avaible system RAM", "Error", MB_ICONEXCLAMATION);
		PostQuitMessage(1);
	}


	//NOTE(Torin) the main memory block is initalized in InitalizeGameMemory
	//by incrementing the pointer past the GameMemory struct. So the avaible memory
	//for the game to use is actutaly config.memorysize - sizeof(GameMemory)
	GameMemory *memory = AllocateGameMemory(&config);
	RenderState *renderState = &memory->renderState;
	InputState *input = &memory->inputState;
	SystemInfo *sys = &memory->systemInfo;

	sys->virtual_memory_size = memoryStatus.ullTotalVirtual;
	SYSTEM_INFO win32SystemInfo;
	GetSystemInfo(&win32SystemInfo);
	sys->cpu_core_count = win32SystemInfo.dwNumberOfProcessors;


#ifdef VENOM_HOTLOAD
	EngineAPI *API = &memory->engineAPI;
#define OpenGLProc(signature, name) API->name = name;
#include "opengl_procs.h"
#undef OpenGLProc
#endif

	U64 performanceFrequency;
	QueryPerformanceFrequency((LARGE_INTEGER *)&performanceFrequency);

	U64 currentTicks;
	U64 lastTicks;
	QueryPerformanceCounter((LARGE_INTEGER *)&currentTicks);
	lastTicks = currentTicks;

	ShowWindow(window, SW_SHOWNORMAL);
	UpdateWindow(window);

	_VenomModuleStart(memory);
	_VenomModuleLoad(memory);

	//XXX REMOVE THIS TEST CODE
	//SoundData musicData = LoadOGG("../assets/music.ogg");
	//PlaySound(&memory->audioState, 100, musicData.sampleCount, musicData.samples);

	while (memory->isRunning) {
		QueryPerformanceCounter((LARGE_INTEGER *)&currentTicks);
		U64 deltaTicks = currentTicks - lastTicks;
		memory->deltaTime = (float)deltaTicks / (float)performanceFrequency;
		lastTicks = currentTicks;

		//TODO(Torin) Make this based on integers rather than floating point
		POINT cursorPoint;
		GetCursorPos(&cursorPoint);
		ScreenToClient(window, &cursorPoint);
		float x = Clamp((float)cursorPoint.x, 0.0f, sys->screen_width);
		float y = Clamp((float)cursorPoint.y, 0.0f, sys->screen_height);
		input->cursorDeltaX = input->cursorPosX - x;
		input->cursorDeltaY = input->cursorPosY - y;
		input->cursorPosX = x;
		input->cursorPosY = y;

		MSG message;
		while (PeekMessage(&message, window, 0, 0, PM_REMOVE)) {
			switch (message.message) {
		
			case WM_QUIT:
			case WM_DESTROY:
			case WM_CLOSE:
			{
				memory->isRunning = false;
			} break;

			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN: 
			case WM_KEYUP:
			{
				int was_down = ((message.lParam & (1 << 30)) != 0);
				int is_down = ((message.lParam & (1 << 31)) == 0);
				input->isKeyDown[message.wParam] = is_down;


        UINT scanCode = MapVirtualKey(message.wParam, MAPVK_VK_TO_VSC);

         
        U16 result = 0;
        ToAscii(message.wParam, scanCode, 0, &result, 0);
        PlatformKeyEventHandler(memory, message.wParam, result, is_down);
			} break;

      case WM_CHAR: {
        ImGuiIO& io = ImGui::GetIO();
        if (message.wParam > 0 && message.wParam < 0x10000)
          io.AddInputCharacter((unsigned short)message.wParam);
      } break;


			case WM_LBUTTONDOWN: {input->isButtonDown[0] = true; } break;
			case WM_LBUTTONUP: { input->isButtonDown[0] = false; } break;
			case WM_RBUTTONDOWN: {input->isButtonDown[1] = true; } break;
			case WM_RBUTTONUP: {input->isButtonDown[1] = false; } break;				

			case WM_MOUSEMOVE:
			{
#if 0
				game_breakpoint(KEYCODE_F9)
				const int x = GET_X_LPARAM(message.lParam);
				const int y = GET_Y_LPARAM(message.lParam);
				input->cursorDeltaX = input->cursorPosX - (float)x;
				input->cursorDeltaY = input->cursorPosY - (float)y;
				input->cursorPosX = (float)x;
				input->cursorPosY = (float)y;
#endif
			} break;

			case WM_PAINT: 
			{
				PAINTSTRUCT paint;
				BeginPaint(window, &paint);
				EndPaint(window, &paint);
			} break;

			}
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		BeginTimedBlock("GameTick");
		BeginTimedBlock("GameUpdate");
		_VenomModuleUpdate(memory);
		EndTimedBlock("GameUpdate");

    BeginTimedBlock("GameRender");
		_VenomModuleRender(memory);
    EndTimedBlock("GameRender");
    EndTimedBlock("GameTick");
		

		SwapBuffers(hdc);


#define AUDIO_BYTES_PER_SAMPLE 2

		static U32 runningSampleIndex = 0;

		AudioState *audioState = &memory->audioState;

#if 0
		{ //@AudioUpdate
			DWORD playCursor, writeCursor;
			secondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor);

			DWORD byteToLock = runningSampleIndex * AUDIO_BYTES_PER_SAMPLE % secondaryBufferSize;
			DWORD bytesToWrite;
			if (byteToLock == playCursor)
			{
				bytesToWrite = secondaryBufferSize;
			}
			else if (byteToLock > playCursor)
			{
				bytesToWrite = (secondaryBufferSize - byteToLock);
				bytesToWrite += playCursor;
			}
			else
			{
				bytesToWrite = playCursor - byteToLock;
			}
			
			assert(bytesToWrite <= AUDIO_OUTPUT_BUFFER_SIZE);
			runningSampleIndex += bytesToWrite / 2;
			MixAudioOutputBuffer(audioState, bytesToWrite);

			VOID *region1, *region2;
			DWORD region1Size, region2Size;
			if (SUCCEEDED(secondaryBuffer->Lock(
				byteToLock, bytesToWrite,
				&region1, &region1Size,
				&region2, &region2Size,
				0)))
			{
				U32 region1SampleCount = region1Size / sizeof(S16);
				U32 region2SampleCount = region2Size / sizeof(S16);
				S16 *bufferA = (S16 *)region1;
				S16 *bufferB = (S16 *)region2;
				if (bufferA != nullptr) {
					memcpy(bufferA, audioState->outputBuffer, region1Size);
				}
				if (bufferB != nullptr) {
					memcpy(bufferB, audioState->outputBuffer + region1SampleCount, region2Size);
				}
				secondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
			}


		}
#endif

	}

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(render_context);
	PostQuitMessage(0);
	return 0;
}

//Fuck you windows
#undef Rectangle
