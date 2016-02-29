#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/stat.h>

#define GLX_RGBA		    4
#define GLX_DOUBLEBUFFER	5
#define GLX_DEPTH_SIZE		12
#define GLX_STENCIL_SIZE	13

typedef struct __GLXcontextRec *GLXContext;
typedef struct __GLXFBConfigRec *GLXFBConfig;
typedef XID GLXDrawable;
typedef XID GLXFBConfigID;

#if 0
extern bool glXQueryVersion(Display *display, int *major, int *minor);
extern XVisualInfo* glXChooseVisual( Display *dpy, int screen,int *attribList );
extern Bool glXMakeCurrent( Display *dpy, GLXDrawable drawable,GLXContext ctx);
extern GLXContext glXCreateContext( Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct );
extern void glXDestroyContext( Display *dpy, GLXContext ctx );
extern void glXSwapBuffers( Display *dpy, GLXDrawable drawable );
#endif

U64 GetPerformanceCounter()
{
	struct timespec Timespec;
	clock_gettime(CLOCK_MONOTONIC, &Timespec);
	U64 result = Timespec.tv_nsec;
	return result;
}

U64 GetPerformanceFrequency()
{
	struct timespec Timespec;
	clock_getres(CLOCK_MONOTONIC, &Timespec);
	U64 result = Timespec.tv_nsec;
	return result;
}

#ifdef VENOM_HOTLOAD
global_variable void *g_game_module;
void InternalLoadGameCode()
{
	g_game_module = dlopen("/home/torin/D/workspace/venom/build/game_module.so", RTLD_LAZY | RTLD_GLOBAL);
	if (g_game_module)
	{
		OnModuleLoad = (OnModuleLoadProc)dlsym(g_game_module, "OnModuleLoad");
		GameStartup = (GameStartupProc)dlsym(g_game_module, "GameStartup");
		GameUpdate = (GameUpdateProc)dlsym(g_game_module, "GameUpdate");
		GameRender = (GameRenderProc)dlsym(g_game_module, "GameRender");
	}
	else
	{
	  LOG_ERROR("Could not find game module");
	} 
}
#endif

U64 GetFileLastWriteTime(const char *filename)
{
	U64 result = 0;
	struct stat file_status;
	if (stat(filename, &file_status) == 0)
	{
		result = file_status.st_mtime;
	}

	return result;
}


int LinuxMain() 
{
	Display *display = XOpenDisplay(NULL);


	UserConfig config = GetUserConfig();

	if (display == NULL)
	{
		return 1;
	}

	//@OpenGL @Init
	void *libgl = dlopen("libGL.so", RTLD_LAZY | RTLD_GLOBAL);
	
	typedef bool(*PROCglXQueryVersion)(Display* display, int *major, int *minor);
	typedef XVisualInfo*(*PROCglXChoseVisual)(Display *display, int screen, int *attribList);
	typedef bool(*PROCglXMakeCurrent)(Display *dpy, GLXDrawable drawable,GLXContext ctx);
	typedef GLXContext(*PROCglXCreateContext)(Display *dpy, XVisualInfo *vis, 
		GLXContext shareList, Bool direct );
	typedef void(*PROCglXDestroyContext)(Display *dpy, GLXContext ctx);
	typedef void(*PROCglXSwapBuffers)(Display *dpy, GLXDrawable drawable);

	PROCglXQueryVersion glXQueryVersion = (PROCglXQueryVersion)dlsym(libgl, "glXQueryVersion");
	PROCglXChoseVisual glXChooseVisual = (PROCglXChoseVisual)dlsym(libgl, "glXChooseVisual");
	PROCglXMakeCurrent glXMakeCurrent = (PROCglXMakeCurrent)dlsym(libgl, "glXMakeCurrent");
	PROCglXCreateContext glXCreateContext = (PROCglXCreateContext)dlsym(libgl, "glXCreateContext");
	PROCglXDestroyContext glXDestroyContext = (PROCglXDestroyContext)dlsym(libgl, "glXDestroyContext");
	PROCglXSwapBuffers glXSwapBuffers = (PROCglXSwapBuffers)dlsym(libgl, "glXSwapBuffers");

#if 0 
	int glx_major, glx_minor;
	int sucuess = glXQueryVersion(display, &glx_major, &glx_minor);
	if (!sucuess) {
		return 1;
	}
#endif

	Window root_window = RootWindow(display, DefaultScreen(display));

#define GLX_RED_SIZE		8
#define GLX_GREEN_SIZE		9
#define GLX_BLUE_SIZE		10
#define GLX_ALPHA_SIZE		11
#define GLX_DEPTH_SIZE		12
#define GLX_STENCIL_SIZE	13

#define GLX_DRAWABLE_TYPE		0x8010
#define GLX_RENDER_TYPE			0x8011

#define GLX_X_RENDERABLE		0x8012
#define GLX_RGBA_BIT			0x00000001
#define GLX_TRUE_COLOR			0x8002
#define GLX_WINDOW_BIT			0x00000001

#define GLX_X_VISUAL_TYPE		0x22

	const int visual_attribs[] = 
	{
		GLX_X_RENDERABLE, true,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
		GLX_RED_SIZE,   8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE,  8,
		GLX_ALPHA_SIZE, 8,
		GLX_DEPTH_SIZE, 24,
		GLX_STENCIL_SIZE, 8,
		GLX_DOUBLEBUFFER, true,
		0
	};


	typedef GLXFBConfig*(*PROCglXChooseFBConfig) 
		(Display *dpy, int screen, int *attrib_list, int *nelements);
	PROCglXChooseFBConfig glXChooseFBConfig = (PROCglXChooseFBConfig)dlsym(libgl, "glXChooseFBConfig");

	typedef XVisualInfo*(*PROCglXGetVisualFromFBConfig)(Display *dpy, GLXFBConfig config);
	PROCglXGetVisualFromFBConfig glXGetVisualFromFBConfig = 
		(PROCglXGetVisualFromFBConfig)dlsym(libgl, "glXGetVisualFromFBConfig");

	int fbcount;
	GLXFBConfig *fbc = glXChooseFBConfig(display, DefaultScreen(display), (int*)visual_attribs, &fbcount);
	GLXFBConfig fbconfig = *fbc;
	XVisualInfo *visual_info = glXGetVisualFromFBConfig(display, fbconfig);
	XFree(fbc);

	Colormap colormap;
	colormap = XCreateColormap(display, root_window, visual_info->visual, AllocNone);

	XSetWindowAttributes window_attribs = {};
	window_attribs.colormap = colormap;
	window_attribs.border_pixel = 0;
	window_attribs.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | 
		PointerMotionMask | ButtonPressMask | ButtonReleaseMask;


	Window window;
	window = XCreateWindow(display, root_window, 0, 0, config.screen_width, config.screen_height, 0, visual_info->depth, 
			InputOutput, visual_info->visual, CWColormap | CWEventMask | CWBorderPixel, &window_attribs);
	XStoreName(display, window, "Venom");
	XMapWindow(display, window);

	typedef GLXContext (*PROCglXCreateContextAttribsARB)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
	PROCglXCreateContextAttribsARB glxCreateContextAttribsARB = 
		(PROCglXCreateContextAttribsARB)dlsym(libgl, "glXCreateContextAttribsARB");

//Context Attributes
#define GLX_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB     0x2092
#define GLX_CONTEXT_FLAGS_ARB             0x2094
#define GLX_CONTEXT_PROFILE_MASK_ARB      0x9126
//Context Flags
#define GLX_CONTEXT_DEBUG_BIT_ARB         0x0001
//Context Mask
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB  0x0001

	int context_flags = 0, context_mask = 0;
	context_flags |= GLX_CONTEXT_DEBUG_BIT_ARB;
	context_mask  |= GLX_CONTEXT_CORE_PROFILE_BIT_ARB;

	const int context_attribs[] = 
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 3,
		GLX_CONTEXT_FLAGS_ARB, context_flags,
		GLX_CONTEXT_PROFILE_MASK_ARB, context_mask,
		0
	};

	GLXContext context;
	context = glxCreateContextAttribsARB(display, fbconfig, NULL, true, context_attribs);
	glXMakeCurrent(display, window, context);



#define _(sig,name) name = (sig)dlsym(libgl, #name);
#include "opengl_procs.h"
#undef _ 

	GameMemory *memory = (GameMemory *)malloc(config.memory_size);
	InitalizeGameMemory(memory, &config);

	GameState *gameState = &memory->gameState;
	InputState *input = &memory->inputState;


	opengl_enable_debug(&memory->debug_memory.debugLog);

	LoadGameCode();
	OnModuleLoad(memory);
	GameStartup(memory);
	
	while (gameState->isRunning)
	{

		platform_independent_tick(memory);

		//TODO(Torin) Do somthing about when the pointer goes out of the screen bounds?
		Window rootReturn, childReturn;
		int rootXReturn, rootYReturn;
		int winXReturn, winYReturn;
		unsigned int maskReturn;
		
		XQueryPointer(display, window, &rootReturn, &childReturn,
				&rootXReturn, &rootYReturn, &winXReturn, &winYReturn, &maskReturn);
		
		input->cursorDeltaX = input->cursorPosX - winXReturn;
		input->cursorDeltaY = input->cursorPosY - winYReturn;
		input->cursorPosX = winXReturn;
		input->cursorPosY = winYReturn;

		input->keysPressedCount = 0;
		
		XEvent event;
		int currentEventCount = XPending(display);
		while (currentEventCount > 0)
		{
			XNextEvent(display, &event);
			switch (event.type) 
			{
				case Expose: 
				{
				} break;
				
				case ButtonPress: 
				{
					input->isButtonDown[event.xbutton.button] = true;
				} break;
				
				case ButtonRelease: 
				{
					input->isButtonDown[event.xbutton.button] = false;
				} break;
				
				case MotionNotify:
				{
				} break;

				case KeyPress:
				{
					input->isKeyDown[event.xkey.keycode] = true;
					input->keycodes[input->keysPressedCount++] = event.xkey.keycode;
					platform_keyevent_proc(memory, event.xkey.keycode, 1);

				} break;
				case KeyRelease:
				{
					input->isKeyDown[event.xkey.keycode] = false;
					platform_keyevent_proc(memory, event.xkey.keycode, 0);

				} break;
			}
			currentEventCount--;
		}

		GameUpdate(memory);
		GameRender(memory);
		glXSwapBuffers(display, window);
		platform_frame_end_proc(memory);
	}
	
	glXMakeCurrent(display, None, NULL);
	glXDestroyContext(display, context);
	XDestroyWindow(display, window);
	XCloseDisplay(display);
	return 1;
}

