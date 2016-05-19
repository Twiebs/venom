#include "renderer_data.h"

namespace UniformLocation {
	static const U32 model = 0;
	static const U32 view = 1;
	static const U32 projection = 2;
	static const U32 light_space = 3;

};

struct VenomDrawCommand {
  U32 vertexArrayID;
  U32 indexCount;
};

struct VenomModelDrawCommand {
  U32 modelID;
  M4 modelMatrix;
};

struct VenomMeshDrawCommand {
  U32 vertexArrayID;
  U32 materialID;
  U32 indexCount;
  U32 indexOffset;
};

struct VenomDebugRenderSettings {
  bool isWireframeEnabled;
  bool isDebugCameraActive;
  bool disableCascadedShadowMaps;
  bool renderDebugNormals;
  bool renderFromDirectionalLight;
  bool drawDepthMap;
};

struct VenomDebugRenderInfo {
  U64 totalVerticesDrawn;
  U64 totalDrawCalls;
  U64 totalDrawListCommandsExecuted;
  U64 totalDrawListsRendered;
  U64 pointLightCount;
  U64 directionalLightCount;
  U64 shadowCastingPointLightCount;
};

struct DirectionalLight {
	V3 direction;
	V3 color;
};

struct PointLight {
	V3 position;
	V3 color;
  F32 radius;
};

struct Frustum {
	F32 field_of_view;
	F32 aspect_ratio;
	F32 near_plane_distance;
	F32 far_plane_distance;
	V3 points[8];
};

struct Camera {
	V3 position;
	V3 front;
	F32 fov;
	F32 yaw, pitch;
	F32 near_clip, far_clip;
  F32 viewportWidth, viewportHeight;
	M4 view, projection;
};

struct VenomDrawList {
  U32 drawCommandCount;
  VenomDrawCommand drawCommands[1000];
  U32 modelDrawComandCount;
  VenomModelDrawCommand modelDrawCommands[1000];
  U32 meshDrawCommandCount;
  VenomMeshDrawCommand meshDrawCommands[1000];

  U32 directionalLightCount;
  DirectionalLight directionalLights[DIRECTIONAL_LIGHTS_MAX];
  U32 pointLightCount;
  PointLight pointLights[POINT_LIGHTS_MAX];
  U32 shadowCastingPointLightCount;
  PointLight shadowCastingPointLights[SHADOW_CASTING_POINT_LIGHT_MAX];
};

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
  light.direction = direction;
  light.color = color;
}

struct Lighting {
	DirectionalLight directionalLights[DIRECTIONAL_LIGHTS_MAX];
	PointLight pointLights[POINT_LIGHTS_MAX];
	U32 directionalLightCount;
	U32 pointLightCount;
};

inline void 
InitializeCamera(Camera *camera, float fov, 
  float near_clip, float far_clip,
  float viewportWidth, float viewportHeight)
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
  camera->viewportWidth = viewportWidth;
  camera->viewportHeight = viewportHeight;
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
#include "opengl_render.h"
#endif//VENOM_OPENGL

