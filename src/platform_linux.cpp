#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/stat.h>


typedef struct __GLXcontextRec *GLXContext;
typedef struct __GLXFBConfigRec *GLXFBConfig;
typedef XID GLXDrawable;
typedef XID GLXFBConfigID;

U64 GetPerformanceCounter() {
	struct timespec Timespec;
	clock_gettime(CLOCK_MONOTONIC, &Timespec);
	U64 result = Timespec.tv_nsec;
	return result;
}

U64 GetPerformanceFrequency() {
	struct timespec Timespec;
	clock_getres(CLOCK_MONOTONIC, &Timespec);
	U64 result = Timespec.tv_nsec;
	return result;
}

U64 GetFileLastWriteTime(const char *filename) {
	U64 result = 0;
	struct stat file_status;
	if (stat(filename, &file_status) == 0) {
		result = file_status.st_mtime;
	}
	return result;
}




#ifdef VENOM_HOTLOAD
void LoadVenomModule(VenomModule* module, const char* path) {
	module->handle = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
	if (module->handle != NULL) {
		VenomModuleLoad   = (VenomModuleLoadProc)dlsym(module->handle, "_VenomModuleLoad");
		VenomModuleStart  = (VenomModuleStartProc)dlsym(module->handle, "_VenomModuleStart");
		VenomModuleUpdate = (VenomModuleUpdateProc)dlsym(module->handle, "_VenomModuleUpdate");
		VenomModuleRender = (VenomModuleRenderProc)dlsym(module->handle, "_VenomModuleRender");
	} else { 
    LOG_ERROR("Could not find game module");
	} 
}

void UnloadVenomModule(VenomModule* module) {
  assert(module->handle != NULL);
  dlclose(module->handle);
  VenomModuleLoad = 0;
  VenomModuleStart = 0;
  VenomModuleUpdate = 0;
  VenomModuleRender = 0;
}
#endif
static inline
void* LoadOpenglAndGetVisualInfo(Display* display, 
    XVisualInfo** out_vi, GLXFBConfig** out_fbc) {
	void *libgl = dlopen("libGL.so", RTLD_LAZY | RTLD_GLOBAL);
  if (libgl == nullptr) {
    LOG_ERROR("Could not find libGL.so");
    return libgl;
  }

	typedef bool(*PROCglXQueryVersion)
    (Display* display, int *major, int *minor);
	typedef XVisualInfo*(*PROCglXChoseVisual)
    (Display *display, int screen, int *attribList);
	PROCglXQueryVersion glXQueryVersion = 
    (PROCglXQueryVersion)dlsym(libgl, "glXQueryVersion");
	PROCglXChoseVisual glXChooseVisual = 
    (PROCglXChoseVisual)dlsym(libgl, "glXChooseVisual");

  static const U32 GLX_RED_SIZE     = 8;
  static const U32 GLX_GREEN_SIZE		= 9;
  static const U32 GLX_BLUE_SIZE		= 10;
  static const U32 GLX_ALPHA_SIZE		= 11;
  static const U32 GLX_DEPTH_SIZE		= 12;
  static const U32 GLX_STENCIL_SIZE	= 13;
  static const U32 GLX_DRAWABLE_TYPE  = 0x8010;
  static const U32 GLX_RENDER_TYPE	  = 0x8011;
  static const U32 GLX_X_RENDERABLE		= 0x8012;
  static const U32 GLX_RGBA_BIT			  = 0x00000001;
  static const U32 GLX_TRUE_COLOR			= 0x8002;
  static const U32 GLX_WINDOW_BIT			= 0x00000001;
  static const U32 GLX_X_VISUAL_TYPE  = 0x22;
  static const U32 GLX_DOUBLEBUFFER	= 5;

//#define GLX_RGBA		    4


  const int visual_attribs[] = {
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
	PROCglXChooseFBConfig glXChooseFBConfig = 
    (PROCglXChooseFBConfig)dlsym(libgl, "glXChooseFBConfig");
	typedef XVisualInfo*(*PROCglXGetVisualFromFBConfig)(Display *dpy, GLXFBConfig config);
	PROCglXGetVisualFromFBConfig glXGetVisualFromFBConfig = 
		(PROCglXGetVisualFromFBConfig)dlsym(libgl, "glXGetVisualFromFBConfig");


  //TODO(Torin) Why is this not done anymore?  
#if 0 
	int glx_major, glx_minor;
	int sucuess = glXQueryVersion(display, &glx_major, &glx_minor);
	if (!sucuess) {
		return 1;
	}
#endif

	int fbcount;
	GLXFBConfig *fbc = glXChooseFBConfig(display, 
    DefaultScreen(display), (int*)visual_attribs, &fbcount);
	GLXFBConfig fbconfig = *fbc;
	XVisualInfo *visual_info = glXGetVisualFromFBConfig(display, fbconfig);

  *out_vi = visual_info;
  *out_fbc = fbc;
  return libgl;
}

static inline
GLXContext CreateOpenglContext(Display* display, Window window, 
  GLXFBConfig* fbc, void* libglSO) {
  
  typedef GLXContext (*PROCglXCreateContextAttribsARB)
    (Display*, GLXFBConfig, GLXContext, Bool, const int*);
	PROCglXCreateContextAttribsARB glxCreateContextAttribsARB = 
		(PROCglXCreateContextAttribsARB)dlsym(libglSO, "glXCreateContextAttribsARB");
  
  static const U32 GLX_CONTEXT_MAJOR_VERSION_ARB     = 0x2091;
  static const U32 GLX_CONTEXT_MINOR_VERSION_ARB     = 0x2092;
  static const U32 GLX_CONTEXT_FLAGS_ARB             = 0x2094;
  static const U32 GLX_CONTEXT_PROFILE_MASK_ARB      = 0x9126;
  static const U32 GLX_CONTEXT_DEBUG_BIT_ARB         = 0x0001;
  static const U32 GLX_CONTEXT_CORE_PROFILE_BIT_ARB  = 0x0001;

	int context_flags = 0, context_mask = 0;
	context_flags |= GLX_CONTEXT_DEBUG_BIT_ARB;
	context_mask  |= GLX_CONTEXT_CORE_PROFILE_BIT_ARB;
	const int context_attribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 3,
		GLX_CONTEXT_FLAGS_ARB, context_flags,
		GLX_CONTEXT_PROFILE_MASK_ARB, context_mask,
		0
	};

	GLXContext context;
	context = glxCreateContextAttribsARB(display, *fbc, NULL, true, context_attribs);
	XFree(fbc);

#define _(sig,name) name = (sig)dlsym(libglSO, #name);
#include "opengl_procedures.h"
#undef _ 

  return context;
}


int LinuxMain() {
	Display *display = XOpenDisplay(NULL);
	if (display == NULL) {
		return 1;
	}

	UserConfig config = GetUserConfig();
	Window root_window = RootWindow(display, DefaultScreen(display));
  XVisualInfo* visualInfo;
  GLXFBConfig* fbConfig;
  
  void* libglSO = LoadOpenglAndGetVisualInfo(display, &visualInfo, &fbConfig);
	typedef void(*PROCglXDestroyContext)(Display *dpy, GLXContext ctx);
  PROCglXDestroyContext glXDestroyContext = 
    (PROCglXDestroyContext)dlsym(libglSO, "glXDestroyContext");
	typedef void(*PROCglXSwapBuffers)(Display *dpy, GLXDrawable drawable);
  PROCglXSwapBuffers glXSwapBuffers = 
    (PROCglXSwapBuffers)dlsym(libglSO, "glXSwapBuffers");

	Colormap colormap;
	colormap = XCreateColormap(display, root_window, visualInfo->visual, AllocNone);

  XSetWindowAttributes window_attribs = {};
	window_attribs.colormap = colormap;
	window_attribs.border_pixel = 0;
	window_attribs.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | 
		PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
	Window window = XCreateWindow(
    display, root_window, 
    0, 0, //x,y 
    config.screen_width, config.screen_height, //width,height
    0, visualInfo->depth, //border_width, depth
    InputOutput, visualInfo->visual, //class, visual
    CWColormap | CWEventMask | CWBorderPixel, //what attribs are being passed
    &window_attribs);
	XStoreName(display, window, "Venom");
	XMapWindow(display, window);
  
  typedef bool(*PROCglXMakeCurrent)
    (Display *dpy, GLXDrawable drawable,GLXContext ctx);
  PROCglXMakeCurrent glXMakeCurrent = 
    (PROCglXMakeCurrent)dlsym(libglSO, "glXMakeCurrent");

  GLXContext context = CreateOpenglContext(display, window, fbConfig, libglSO);
	glXMakeCurrent(display, window, context);

#if 0
  if (config.is_fullscreen) {
    Atom wmStateAtom = XInternAtom(display, "_NET_WM_STATE", 0); 
    Atom fullcreenAtom = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", 0);

    Atom property =
    XChangeProperty(display, window,  property, 0),
      XA_ATOM, 32, PropModeReplace, atoms, 1);
  }
#endif

	GameMemory *memory = AllocateGameMemory(&config);
	GameState *gameState = &memory->gameState;
	InputState *input = &memory->inputState;
	opengl_enable_debug(&memory->debug_memory.debugLog);

  VenomModule module = {};
  LoadVenomModule(&module, "../build/game_module.so");
  if (module.handle != NULL) {
    VenomModuleStart(memory);
    assert(memory->mainBlock.size > memory->mainBlock.used);
    U64 remainingBlockMemory = memory->mainBlock.size - memory->mainBlock.used;
    InitSubBlock("AssetCache", &memory->assets.memory, 
        remainingBlockMemory, &memory->mainBlock);
    U64 terrainBlockEnd = (U64)(memory->terrainGenState.memory.base 
      + memory->terrainGenState.memory.used);
    assert(terrainBlockEnd < (U64)memory->assets.memory.base);
    VenomModuleLoad(memory);
  } else {
    return 1;
  }
		
	while (gameState->isRunning) {
		platform_independent_tick(memory, &module);

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
				case Expose: {
				} break;
				
				case ButtonPress: {
					input->isButtonDown[event.xbutton.button] = true;
				} break;
				
				case ButtonRelease: {
					input->isButtonDown[event.xbutton.button] = false;
				} break;
				
				case MotionNotify: {
				} break;

				case KeyPress: {
					input->isKeyDown[event.xkey.keycode] = true;
					input->keycodes[input->keysPressedCount++] = event.xkey.keycode;
					platform_keyevent_proc(memory, event.xkey.keycode, 1);
				} break;
				case KeyRelease: {
					input->isKeyDown[event.xkey.keycode] = false;
					platform_keyevent_proc(memory, event.xkey.keycode, 0);
				} break;

			}
			currentEventCount--;
		}

		VenomModuleUpdate(memory);
		VenomModuleRender(memory);
		glXSwapBuffers(display, window);
		platform_frame_end_proc(memory);
	}

  //The Allocated Game Memory is intentionaly not freed here  
	glXMakeCurrent(display, None, NULL);
	glXDestroyContext(display, context);
	XDestroyWindow(display, window);
	XCloseDisplay(display);
  return 0;
}

