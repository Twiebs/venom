#include "renderer_data.h"



struct ModelAsset;

struct VenomDrawCommand {
  U32 vertexArrayID;
  U32 indexCount;
};

struct VenomModelDrawCommand {
  ModelAsset *model;
  M4 modelMatrix;
  //TODO(Torin: May 31, 2016) Remove these when done testing
  V3 position;
  V3 rotation;
  V3 scale;
};

struct Animated_Model_Draw_Command {
  ModelAsset *model;
  AnimationState *animation_state;
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

  B8 drawDepthMap;
  B8 drawSkeletons;

  B8 drawBroadphaseVolumes;
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

inline void SetUniform(S32 location, S32 value);
inline void SetUniform(S32 location, F32 value);
inline void SetUniform(S32 location, V2 value);
inline void SetUniform(S32 location, V3 value);
inline void SetUniform(S32 location, V4 value);
inline void SetUniform(S32 location, M4 value);

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





#ifdef VENOM_OPENGL
#include "thirdparty/glcorearb.h"
#include "opengl_resources.h"
#include "assets/offline_asset_tools.h"
#endif//VENOM_OPENGL

