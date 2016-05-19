#include "renderer_data.cpp"

#ifdef VENOM_OPENGL
#ifdef VENOM_HOTLOAD
#define _(signature, name) static signature name;
#include "opengl_procedures.h"
#undef _ //Create Opengl global function pointers
#endif //VENOM_HOTLOAD
#include "opengl_debug.h"

#include "opengl_glsl.cpp"
#include "opengl_resources.cpp"
#include "opengl_render.cpp"
#endif//VENOM_OPENGL
