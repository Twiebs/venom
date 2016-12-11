#include "renderer_data.h"



struct ModelAsset;

struct VenomDrawCommand {
  U32 vertexArrayID;
  U32 indexCount;
};

struct VenomModelDrawCommand {
  U32 modelID;
  M4 modelMatrix;
  //TODO(Torin: May 31, 2016) Remove these when done testing
  V3 position;
  V3 rotation;
  V3 scale;
};



struct Animated_Model_Draw_Command {
  ModelAsset *model;
  Animation_State *animation_state;
  M4 model_matrix;
};

struct VenomMeshDrawCommand {
  U32 vertexArrayID;
  U32 materialID;
  U32 indexCount;
  U32 indexOffset;
};


#ifndef VENOM_RELEASE
struct VenomDebugRenderSettings {
  B8 isWireframeEnabled;
  B8 isDebugCameraActive;
  B8 disableCascadedShadowMaps;
  B8 disableAtmosphere;
  B8 renderDebugNormals;
  B8 renderFromDirectionalLight;
  B8 drawPhysicsColliders;
  B8 drawDepthMap;
};

struct VenomDebugRenderFrameInfo {
  U64 totalVerticesDrawn;
  U64 totalDrawCalls;
  U64 totalDrawListCommandsExecuted;
  U64 totalDrawListsRendered;
  U64 pointLightCount;
  U64 directionalLightCount;
  U64 shadowCastingPointLightCount;
};
#endif//VENOM_RELEASE

struct VenomDrawList {
  U32 drawCommandCount;
  VenomDrawCommand drawCommands[1000];
  U32 modelDrawComandCount;
  VenomModelDrawCommand modelDrawCommands[1000];
  U32 meshDrawCommandCount;
  VenomMeshDrawCommand meshDrawCommands[1000];
  U32 outlinedModelDrawCommandCount;
  VenomModelDrawCommand outlinedModelDrawCommands[8];

  DynamicArray<Animated_Model_Draw_Command> animated_model_draw_commands;

  U32 directionalLightCount;
  DirectionalLight directionalLights[DIRECTIONAL_LIGHTS_MAX];
  U32 pointLightCount;
  PointLight pointLights[POINT_LIGHTS_MAX];
  U32 shadowCastingPointLightCount;
  PointLight shadowCastingPointLights[SHADOW_CASTING_POINT_LIGHT_MAX];
};

static inline void set_uniform(S32 location, S32 value);
static inline void set_uniform(S32 location, F32 value);
static inline void set_uniform(S32 location, V2 value);
static inline void set_uniform(S32 location, V3 value);
static inline void set_uniform(S32 location, V4 value);
static inline void set_uniform(S32 location, M4 value);

inline void 
AddPointLight(const V3 position, const V3 color, const F32 radius, VenomDrawList* list) {
  assert(list->pointLightCount + 1 <= (int)ARRAY_COUNT(list->pointLights));
  PointLight& light = list->pointLights[list->pointLightCount++];
  light.position = position;
  light.color = color;
  light.radius = radius;
}

inline void 
AddShadowCastingPointLight(const V3 position, const V3 color, 
  const F32 radius, VenomDrawList* list) {
  assert(list->shadowCastingPointLightCount + 1 <= 
    (int)ARRAY_COUNT(list->shadowCastingPointLights));
  PointLight& light = list->shadowCastingPointLights[list->shadowCastingPointLightCount++];
  light.position = position;
  light.color = color;
  light.radius = radius;
}

inline void 
AddDirectionalLight(const V3 direction, const V3 color, VenomDrawList* list) {
  assert(list->directionalLightCount + 1 <= (int)ARRAY_COUNT(list->directionalLights));
  DirectionalLight& light = list->directionalLights[list->directionalLightCount++];
  light.direction = Normalize(direction);
  light.color = color;
}

inline void 
InitializeCamera(Camera *camera, float fov, 
  float near_clip, float far_clip, float viewportWidth, float viewportHeight) 
{
	camera->position = V3(0.0f, 0.0f, 0.0f);
	camera->front = V3(0.0f, 0.0f, 0.0f);
	camera->fov = fov;
	camera->yaw = 0.0f;
	camera->pitch = 0.0f;
	camera->near_clip = near_clip;
	camera->far_clip = far_clip;
  camera->projection = Perspective(camera->fov, viewportWidth, viewportHeight,
    camera->near_clip, camera->far_clip);
  //camera->viewportWidth = viewportWidth;
  //camera->viewportHeight = viewportHeight;
}

inline void 
UpdateCamera(Camera *camera) {
	camera->front.x = cosf(camera->yaw) * cosf(camera->pitch);
	camera->front.y = sinf(camera->pitch);
	camera->front.z = sin(camera->yaw) * cos(camera->pitch);
	camera->front = Normalize(camera->front);
  camera->view = LookAt(camera->position, camera->front + camera->position, V3(0.0f, 1.0f, 0.0f));
}

#ifdef VENOM_OPENGL
#include "glcorearb.h"
#include "opengl_resources.h"
#include "offline_asset_tools.h"
#endif//VENOM_OPENGL

