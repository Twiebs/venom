
struct CascadedShadowMap {
  GLuint framebuffer;
  GLuint depthTexture;
  Frustum cascadeFrustums[SHADOW_MAP_CASCADE_COUNT];
  M4 lightSpaceTransforms[SHADOW_MAP_CASCADE_COUNT];
  M4 lightProjectionMatrices[SHADOW_MAP_CASCADE_COUNT];
	F32 shadowCascadeDistances[SHADOW_MAP_CASCADE_COUNT];
};

struct OmnidirectionalShadowMap {
  GLuint depthCubemap;
};

struct GBuffer {
  GLuint framebuffer;
  GLuint positionDepth;
  GLuint normal;
  GLuint albedoSpecular;
};



