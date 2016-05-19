#include "renderer_data.cpp"

#ifdef VENOM_OPENGL
#define _(signature, name) static signature name;
#include "opengl_procedures.h"
#undef _
#include "opengl_glsl.cpp"
#include "opengl_resources.cpp"
#include "opengl_render.cpp"
#include "opengl_debug.cpp"
#include "offline_asset_tools.cpp"
#endif//VENOM_OPENGL