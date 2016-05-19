inline void 
InitGBuffer(GBuffer* gbuffer, const U32 viewportWidth, const U32 viewportHeight) {
  glGenFramebuffers(1, &gbuffer->framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, gbuffer->framebuffer);
  
  glGenTextures(1, &gbuffer->positionDepth);
  glBindTexture(GL_TEXTURE_2D, gbuffer->positionDepth);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, viewportWidth, viewportHeight);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glGenTextures(1, &gbuffer->normal);
  glBindTexture(GL_TEXTURE_2D, gbuffer->normal);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, viewportWidth, viewportHeight);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  
  glGenTextures(1, &gbuffer->albedoSpecular);
  glBindTexture(GL_TEXTURE_2D, gbuffer->albedoSpecular);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, viewportWidth, viewportHeight);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
    GL_TEXTURE_2D, gbuffer->positionDepth, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, 
    GL_TEXTURE_2D, gbuffer->normal, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, 
    GL_TEXTURE_2D, gbuffer->albedoSpecular, 0);

  GLuint attachments[3] = { 
    GL_COLOR_ATTACHMENT0, 
    GL_COLOR_ATTACHMENT1, 
    GL_COLOR_ATTACHMENT2
  };

  glDrawBuffers(3, attachments);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
}


void InitCascadedShadowMaps(CascadedShadowMap* csm, 
  F32 viewportWidth, F32 viewportHeight, F32 fieldOfView) 
{ 
  glGenFramebuffers(1, &csm->framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, csm->framebuffer);
  glDrawBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glGenTextures(1, &csm->depthTexture);
  glBindTexture(GL_TEXTURE_2D_ARRAY, csm->depthTexture);
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT,
               SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION, 
               SHADOW_MAP_CASCADE_COUNT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  for (U32 i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
    csm->cascadeFrustums[i].aspect_ratio = 
      viewportWidth / viewportHeight;
    csm->cascadeFrustums[i].field_of_view = fieldOfView + 0.2f;
  }
}
static const U32 OMNI_SHADOW_MAP_RESOLUTION = 4096;

void InitOmnidirectionalShadowMap(OmnidirectionalShadowMap* osm) {
  glGenTextures(1, &osm->depthCubemap);
  glBindTexture(GL_TEXTURE_CUBE_MAP, osm->depthCubemap);
  for (size_t i = 0; i < 6; i++)
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
      OMNI_SHADOW_MAP_RESOLUTION, OMNI_SHADOW_MAP_RESOLUTION, 0, 
      GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);  
}



static inline
void UpdateCSMFustrums(CascadedShadowMap* csm, Camera* camera) 
{
  Frustum *f = csm->cascadeFrustums;
  float clip_plane_distance_ratio = camera->far_clip / camera->near_clip;
  f[0].near_plane_distance = camera->near_clip;
  for (U32 i = 1; i < SHADOW_MAP_CASCADE_COUNT; i++) {
    float scalar = (float)i / (float)SHADOW_MAP_CASCADE_COUNT;
    f[i].near_plane_distance = SHADOW_MAP_CASCADE_WEIGHT *
      (camera->near_clip * powf(clip_plane_distance_ratio, scalar)) +
      (1 - SHADOW_MAP_CASCADE_WEIGHT) * (camera->near_clip + 
      (camera->far_clip - camera->near_clip) * scalar);
    f[i - 1].far_plane_distance = f[i].near_plane_distance * SHADOW_MAP_CASCADE_TOLERANCE;
  }
  f[SHADOW_MAP_CASCADE_COUNT - 1].far_plane_distance = camera->far_clip;
}

static inline
void BindMaterial(const MaterialDrawable& material) {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, material.diffuse_texture_id);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, material.normal_texture_id);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, material.specular_texture_id);
};

static inline 
void DrawTerrainGeometry(TerrainGenerationState *terrainGenState) {
  glActiveTexture(GL_TEXTURE5);
  glBindTexture(GL_TEXTURE_2D_ARRAY, terrainGenState->heightmap_texture_array);
  glActiveTexture(GL_TEXTURE6);
  glBindTexture(GL_TEXTURE_2D_ARRAY, terrainGenState->normals_texture_array);
  glActiveTexture(GL_TEXTURE7);
  glBindTexture(GL_TEXTURE_2D_ARRAY, terrainGenState->detailmap_texture_array);

  glBindVertexArray(terrainGenState->base_mesh.vertexArrayID);
  glDrawElementsInstanced(GL_TRIANGLES, TERRAIN_INDEX_COUNT_PER_CHUNK, 
    GL_UNSIGNED_INT, 0, TERRAIN_TOTAL_CHUNK_COUNT);
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
};

inline RenderGroup CreateDebugRenderGroup() {
	RenderGroup result = {};

	glGenVertexArrays(1, &result.vao);
	glGenBuffers(1, &result.vbo);
	glGenBuffers(1, &result.ebo);

	glBindVertexArray(result.vao);
	glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V3) + sizeof(V4), (GLvoid*)0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(V3) + sizeof(V4), (GLvoid*)sizeof(V3));
	return result;
}

inline RenderGroup CreateImGuiRenderGroup() {
	RenderGroup result = {};
	glGenVertexArrays(1, &result.vao);
	glGenBuffers(1, &result.vbo);
	glGenBuffers(1, &result.ebo);

	glBindVertexArray(result.vao);
	glBindBuffer(GL_ARRAY_BUFFER, result.vbo);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, pos));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, uv));
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, col));
	glBindVertexArray(0);
	return result;
}

inline void FreeRenderGroup(RenderGroup *group) {
	glDeleteBuffers(1, &group->vao);
	glDeleteBuffers(1, &group->ebo);
	glDeleteVertexArrays(1, &group->vao);
}
#if 0
void DrawIndexedVertexArray(IndexedVertexArray* vertexArray) {
  assert(vertexArray->vertexArrayID != 0);
  assert(vertexArray->indexCount != 0);
  glBindVertexArray(vertexArray->vertexArrayID);
  glDrawElements(GL_TRIANGLES, vertexArray->indexCount, GL_UNSIGNED_INT, 0);
}
#endif

void PushDrawCommand(IndexedVertexArray* vertexArray, VenomDrawList* drawList) {
  assert(drawList->drawCommandCount + 1 < ARRAY_COUNT(drawList->drawCommands));
  VenomDrawCommand* drawCommand = &drawList->drawCommands[drawList->drawCommandCount++];
  drawCommand->vertexArrayID = vertexArray->vertexArrayID;
  drawCommand->indexCount = vertexArray->indexCount;
}

static inline
void SetLightingUniforms(const VenomDrawList* drawList, const Camera& camera) {
  static const U32 cameraViewPositionUniformLocation = 4;
  static const U32 directionalLightCountUniformLocation = 5;
  static const U32 pointLightCountUniformLocation = 6;
  static const U32 shadowCastingPointLightLocation = 7; 

  glUniform3f(cameraViewPositionUniformLocation,
    camera.position.x, camera.position.y, camera.position.z);
  glUniform1i(directionalLightCountUniformLocation, 
    (int)drawList->directionalLightCount);
  glUniform1i(pointLightCountUniformLocation,
    (int)drawList->pointLightCount);
  glUniform1i(shadowCastingPointLightLocation, 
    (int)drawList->shadowCastingPointLightCount);

  for (size_t i = 0; i < drawList->directionalLightCount; i++) {
    const DirectionalLight& light = drawList->directionalLights[i];
    glUniform3f(DIRECTIONAL_LIGHT_UNIFORM_LOCATION + 0 + 
      (i*UNIFORM_COUNT_PER_DIRECTIONAL_LIGHT), light.direction.x,
      light.direction.y, light.direction.z);
    glUniform3f(DIRECTIONAL_LIGHT_UNIFORM_LOCATION + 1 + 
      (i*UNIFORM_COUNT_PER_DIRECTIONAL_LIGHT), light.color.x,
      light.color.y, light.color.z);		
  }

  for (size_t i = 0; i < drawList->pointLightCount; i++) {
    const PointLight& light = drawList->pointLights[i];
    glUniform3f(POINT_LIGHT_UNIFORM_LOCATION + 0 +
      (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.position.x,
      light.position.y, light.position.z);
    glUniform3f(POINT_LIGHT_UNIFORM_LOCATION + 1 +
      (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.color.x,
      light.color.y, light.color.z);
    glUniform1f(POINT_LIGHT_UNIFORM_LOCATION + 2 +
      (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.radius);
  }

  for (size_t i = 0; i < drawList->shadowCastingPointLightCount; i++) {
    const PointLight& light = drawList->shadowCastingPointLights[i];
    glUniform3f(SHADOW_CASTING_POINT_LIGHT_UNIFORM_LOCATION + 0 +
      (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.position.x,
      light.position.y, light.position.z);
    glUniform3f(SHADOW_CASTING_POINT_LIGHT_UNIFORM_LOCATION + 1 +
      (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.color.x,
      light.color.y, light.color.z);
    glUniform1f(SHADOW_CASTING_POINT_LIGHT_UNIFORM_LOCATION + 2 +
      (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.radius);
  }
}


static VenomDebugRenderInfo* _debugRenderInfo;

static inline
void RenderDrawList(VenomDrawList* drawList, GameAssets* assets) {
#if 1
  for (size_t i = 0; i < drawList->drawCommandCount; i++) {
    VenomDrawCommand* drawCmd = &drawList->drawCommands[i];
    glBindVertexArray(drawCmd->vertexArrayID);
    glDrawElements(GL_TRIANGLES, drawCmd->indexCount, GL_UNSIGNED_INT, 0);
  }
#endif

  for (size_t i = 0; i < drawList->modelDrawComandCount; i++) {
    VenomModelDrawCommand* drawCmd = &drawList->modelDrawCommands[i];
    const ModelDrawable& drawable = 
      GetModelDrawable((DEBUGModelID)drawCmd->modelID, assets);
    glUniformMatrix4fv(0, 1, GL_FALSE, &drawCmd->modelMatrix[0][0]);
   U64 currentIndexOffset = 0;
    glBindVertexArray(drawable.vertexArrayID);
    for (U64 j = 0; j < drawable.meshCount; j++) {
      const MaterialDrawable &material = drawable.materials[j];
      BindMaterial(material);
      glDrawElements(GL_TRIANGLES, drawable.indexCountPerMesh[j], 
        GL_UNSIGNED_INT, (GLvoid*)(sizeof(U32)*currentIndexOffset));
      currentIndexOffset += drawable.indexCountPerMesh[j];
    }
  }

#if 1
  for (size_t i = 0; i < drawList->meshDrawCommandCount; i++) {
    VenomMeshDrawCommand* cmd = &drawList->meshDrawCommands[i];
    const MaterialDrawable& material = GetMaterial(cmd->materialID, assets);
    BindMaterial(material);
    M4 model = M4Identity();
    glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
    glBindVertexArray(cmd->vertexArrayID);
    glDrawElements(GL_TRIANGLES, cmd->indexCount, 
      GL_UNSIGNED_INT, (GLvoid*)(cmd->indexOffset * sizeof(U32)));
  }
#endif

#ifndef VENOM_RELEASE
  _debugRenderInfo->totalDrawListsRendered++;
  _debugRenderInfo->totalDrawListCommandsExecuted += drawList->drawCommandCount;
#endif
}

void PushMeshDrawCommand(U32 vertexArrayID, U32 materialID, 
  U32 indexCount, U32 indexOffset, VenomDrawList* drawList) 
{
  assert(drawList->meshDrawCommandCount + 1 < ARRAY_COUNT(drawList->meshDrawCommands));
  VenomMeshDrawCommand& cmd = drawList->meshDrawCommands[drawList->meshDrawCommandCount++];
  cmd.vertexArrayID = vertexArrayID;
  cmd.materialID = materialID;
  cmd.indexCount = indexCount;
  cmd.indexOffset = indexOffset;
}

void PushModelDrawCommand(DEBUGModelID modelID, V3 position, VenomDrawList* drawList) {
  assert(drawList->modelDrawComandCount + 1 < ARRAY_COUNT(drawList->modelDrawCommands));
  VenomModelDrawCommand& cmd = 
    drawList->modelDrawCommands[drawList->modelDrawComandCount++];
  cmd.modelID = modelID;
  cmd.modelMatrix = Translate(position);
}


static inline
void RenderCSM(CascadedShadowMap* csm, Camera* camera, 
  DirectionalLight* light, VenomDrawList* drawList, GameAssets* assets) 
{
	M4 light_view = LookAt(V3(0.0f), -light->direction, V3(-1.0f, 0.0f, 0.0f));
	for (U32 i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
		//Compute cascade frustum points	
		Frustum *f = csm->cascadeFrustums + i;
		csm->shadowCascadeDistances[i] = 0.5f * (-f->far_plane_distance * 
      camera->projection[2][2] + camera->projection[3][2]) / 
      f->far_plane_distance + 0.5f;

		V3 up = V3(0.0f, 1.0f, 0.0f);
		V3 view_direction = camera->front;
		V3 center = camera->position;
		V3 right = Cross(view_direction, up);

		right = Normalize(right);
		up = Normalize(Cross(right, view_direction));

		V3 far_plane_center = center + (view_direction * f->far_plane_distance);
		V3 near_plane_center = center + (view_direction * f->near_plane_distance);

		float tan_half_fov = tanf(f->field_of_view / 2.0f);
		float near_half_height = tan_half_fov * f->near_plane_distance;
		float near_half_width = near_half_height * f->aspect_ratio;
		float far_half_height = tan_half_fov * f->far_plane_distance;
		float far_half_width = far_half_height * f->aspect_ratio;

		f->points[0] = near_plane_center - (up * near_half_height) - (right * near_half_width);
		f->points[1] = near_plane_center + (up * near_half_height) - (right * near_half_width);
		f->points[2] = near_plane_center + (up * near_half_height) + (right * near_half_width);
		f->points[3] = near_plane_center - (up * near_half_height) + (right * near_half_width);

		f->points[4] = far_plane_center - (up * far_half_height) - (right * far_half_width);
		f->points[5] = far_plane_center + (up * far_half_height) - (right * far_half_width);
		f->points[6] = far_plane_center + (up * far_half_height) + (right * far_half_width);
		f->points[7] = far_plane_center - (up * far_half_height) + (right * far_half_width);

		float minZ, maxZ;
		{
			V4 view_space = light_view * V4(f->points[i], 1.0f);
			minZ = view_space.z;
			maxZ = view_space.z;
		}

		for (int i = 1; i < 8; i++) {
			V4 view_space = light_view * V4(f->points[i], 1.0f);
			maxZ = view_space.z > maxZ ? view_space.z : maxZ;
			minZ = view_space.z < minZ ? view_space.z : minZ;
		}

		//TODO(Torin) Insure all loaded objects that cast shadows fall inside the frustum 

		csm->lightProjectionMatrices[i] = Orthographic(-1.0f, 1.0f, 
      -1.0f, 1.0f, -maxZ, -minZ, 1.0);
		M4 light_transform = csm->lightProjectionMatrices[i] * light_view;

		float maxX = -1000000, minX = 1000000;
		float maxY = -1000000, minY = 1000000;
		for (U32 i = 0; i < 8; i++) {
			V4 light_space = light_transform * V4(f->points[i], 1.0f);
			light_space.x /= light_space.w;
			light_space.y /= light_space.w;

			maxX = Max(maxX, light_space.x);
			minX = Min(minX, light_space.x);
			maxY = Max(maxY, light_space.y);
			minY = Min(minY, light_space.y);
		}

		float scaleX = 2.0f / (maxX - minX);
		float scaleY = 2.0f / (maxY - minY);
		float offsetX = -0.5f * (maxX + minX) * scaleX;
		float offsetY = -0.5f * (maxY + minY) * scaleY;

		M4 crop_matrix = M4Identity();
		crop_matrix[0][0] = scaleX;
		crop_matrix[1][1] = scaleY;
		crop_matrix[3][0] = offsetX;
		crop_matrix[3][1] = offsetY;

		csm->lightProjectionMatrices[i] = csm->lightProjectionMatrices[i] * crop_matrix;
		light_transform = csm->lightProjectionMatrices[i] * light_view;
		csm->lightSpaceTransforms[i] = light_transform;

		glViewport(0, 0, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, 
      GL_DEPTH_ATTACHMENT, csm->depthTexture, 0, i);
		glClear(GL_DEPTH_BUFFER_BIT);
		glUniformMatrix4fv(UniformLocation::light_space, 1, 0, &light_transform[0][0]);
    RenderDrawList(drawList, assets);
	}
}

#ifndef VENOM_RELEASE
#define DebugEnableIf(expr) if (expr)
#define DebugDisableIf(expr) if(!expr)
#else//VENOM_RELEASE
#define DebugDisableIf(expr) if (1)
#define DebugEnableIf(expr) if (0)
#endif//VENOM_RELEASE

static inline 
void SetLightSpaceTransformUniforms(CascadedShadowMap* csm, GLuint programID) {
  GLint matrixLocation = glGetUniformLocation(programID, "u_light_space_matrix");
  GLint distanceLocation = glGetUniformLocation(programID, "u_shadow_cascade_distance");
	glUniformMatrix4fv(matrixLocation, SHADOW_MAP_CASCADE_COUNT, 
    GL_FALSE, &csm->lightSpaceTransforms[0][0][0]);
  glUniform1fv(distanceLocation, SHADOW_MAP_CASCADE_COUNT, 
    &csm->shadowCascadeDistances[0]);
}

void VenomRenderScene(GameMemory* memory, Camera* camera) {
  RenderState* rs = &memory->renderState;
  _debugRenderInfo = &rs->debugRenderInfo; 
  GameAssets* assets = &memory->assets;
  SystemInfo* sys = &memory->systemInfo;
  VenomDrawList* drawList = &rs->drawList;
  UpdateCamera(camera);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, sys->screen_width, sys->screen_height);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glDisable(GL_SCISSOR_TEST);

#if 0
  DebugDisableIf(rs->debugRenderSettings.disableCascadedShadowMaps) {
    CascadedShadowMap* csm = &rs->csm;
    GLuint depthMapShader = GetShaderProgram(ShaderID_depth_map, assets);
    glViewport(0, 0, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION);
    glBindFramebuffer(GL_FRAMEBUFFER, csm->framebuffer);
    glUseProgram(depthMapShader);
    UpdateCSMFustrums(csm, camera);
    DirectionalLight* light = &drawList->directionalLights[0];
    //glCullFace(GL_FRONT);
    RenderCSM(csm, camera, light, &rs->drawList, assets);
    //glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, (GLsizei)sys->screen_width, (GLsizei)sys->screen_height);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D_ARRAY, csm->depthTexture);
    glClear(GL_DEPTH_BUFFER_BIT);
  }
#endif

#if 0
  {
    CascadedShadowMap* csm = &rs->csm;
    
    glViewport(0, 0, OMNI_SHADOW_MAP_RESOLUTION, OMNI_SHADOW_MAP_RESOLUTION);
    glBindFramebuffer(GL_FRAMEBUFFER, csm->framebuffer);
    GLuint programID = GetShaderProgram(ShaderID_depth_cubemap, assets);
    glUseProgram(programID);

    for (size_t i = 0; i < drawList->shadowCastingPointLightCount; i++) {
      const PointLight& light = drawList->shadowCastingPointLights[i]; 
      const OmnidirectionalShadowMap* osm = &rs->osm[i];
      static const F32 lightNearPlane = 0.4f;
      static const F32 lightFarPlane = 25.0f;
      
      M4 lightTransformMatrices[6];
      M4 lightProjectionMatrix = Perspective(90.0f * DEG2RAD, OMNI_SHADOW_MAP_RESOLUTION,
        OMNI_SHADOW_MAP_RESOLUTION, lightNearPlane, lightFarPlane);
      lightTransformMatrices[0] = lightProjectionMatrix * 
        LookAt(light.position, light.position + V3(1.0, 0.0, 0.0), V3(0.0, -1.0, 0.0));
      lightTransformMatrices[1] = lightProjectionMatrix * 
        LookAt(light.position, light.position + V3(-1.0, 0.0, 0.0), V3(0.0, -1.0, 0.0));
      lightTransformMatrices[2] = lightProjectionMatrix * 
        LookAt(light.position, light.position + V3(0.0, 1.0, 0.0), V3(0.0, 0.0, 1.0));
      lightTransformMatrices[3] = lightProjectionMatrix * 
        LookAt(light.position, light.position + V3(0.0, -1.0, 0.0), V3(0.0, 0.0, -1.0));
      lightTransformMatrices[4] = lightProjectionMatrix * 
        LookAt(light.position, light.position + V3(0.0, 0.0, 1.0), V3(0.0, -1.0, 0.0));
      lightTransformMatrices[5] = lightProjectionMatrix * 
        LookAt(light.position, light.position + V3(0.0, 0.0, -1.0), V3(0.0, -1.0, 0.0));
      
      static const U32 lightTransformMatricesLocation = 1;
      static const U32 lightPositionLocation = 7;
      static const U32 farPlaneLocation = 8;
      
      glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, osm->depthCubemap, 0);
      glClear(GL_DEPTH_BUFFER_BIT);
      for (size_t i = 0; i < 6; i++) {
        glUniformMatrix4fv(lightTransformMatricesLocation + i, 
          1, GL_FALSE, &lightTransformMatrices[i][0][0]);
      }
      
      glUniform3f(lightPositionLocation,
        light.position.x, light.position.y, light.position.z);
      glUniform1f(farPlaneLocation, lightFarPlane);
      M4 model = M4Identity();
      glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
      RenderDrawList(&rs->drawList, assets);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
    for (size_t i = 0; i < SHADOW_CASTING_POINT_LIGHT_MAX; i++) {
      static const U32 pointLightShadowMapSamplerBindingBase = 4;
      glActiveTexture(GL_TEXTURE0 + pointLightShadowMapSamplerBindingBase + i);
      glBindTexture(GL_TEXTURE_CUBE_MAP, rs->osm[i].depthCubemap);
    }
  }
#endif

#if 1
  glViewport(0, 0, sys->screen_width, sys->screen_height);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  GLuint opaqueShader = GetShaderProgram(ShaderID_material_opaque, assets);
  M4 model = M4Identity();
  M4 view = camera->view;
  M4 projection = camera->projection;
  glUseProgram(opaqueShader);
  glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
	glUniformMatrix4fv(1, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(2, 1, GL_FALSE, &projection[0][0]);
  SetLightingUniforms(&rs->drawList, *camera);
  SetLightSpaceTransformUniforms(&rs->csm, opaqueShader);
  DebugEnableIf(rs->debugRenderSettings.isWireframeEnabled) 
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  RenderDrawList(&rs->drawList, &memory->assets);
  DebugEnableIf(rs->debugRenderSettings.isWireframeEnabled) 
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

#else //Defered Renderer
  GBuffer* gbuffer = &rs->gbuffer;

  glViewport(0, 0, sys->screen_width, sys->screen_height);
  glBindFramebuffer(GL_FRAMEBUFFER, gbuffer->framebuffer);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  GLuint gbufferProgram = GetShaderProgram(ShaderID_GBuffer, assets);
  glUseProgram(gbufferProgram);
  M4 model = M4Identity();
  glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
  glUniformMatrix4fv(1, 1, GL_FALSE, &camera->view[0][0]);
	glUniformMatrix4fv(2, 1, GL_FALSE, &camera->projection[0][0]);
  RenderDrawList(drawList, assets);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  GLuint deferredMaterialShader = GetShaderProgram(ShaderID_DeferredMaterial, assets);
  glUseProgram(deferredMaterialShader);
  //SetLightingUniforms(drawList, *camera);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, gbuffer->positionDepth);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, gbuffer->normal);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, gbuffer->albedoSpecular);

  glBindBuffer(GL_ARRAY_BUFFER, rs->quadVao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#endif

#if 0
  DebugEnableIf(rs->debugRenderSettings.renderDebugNormals) {
    GLuint shader = GetShaderProgram(ShaderID_debug_normals, assets);
    glUseProgram(shader);
    glUniformMatrix4fv(1, 1, GL_FALSE, &camera->view[0][0]);
    glUniformMatrix4fv(2, 1, GL_FALSE, &camera->projection[0][0]);
    RenderDrawList(&rs->drawList, assets);
		glUseProgram(0);
  }

  DebugEnableIf(rs->debugRenderSettings.drawDepthMap) {
    //glViewport(0, 0, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION);
    GLuint debugDepthMapShader = GetShaderProgram(ShaderID_debug_depth_map, assets);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D_ARRAY, rs->csm.depthTexture);
		glUseProgram(debugDepthMapShader);
		glUniform1i(0, 1);
    glBindBuffer(GL_ARRAY_BUFFER, rs->quadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#warning depthMapVisualizerBroken!
  }
#endif

  _debugRenderInfo->pointLightCount = drawList->pointLightCount;
  _debugRenderInfo->directionalLightCount = drawList->directionalLightCount;
  _debugRenderInfo->shadowCastingPointLightCount = drawList->shadowCastingPointLightCount;
  drawList->drawCommandCount = 0;
  drawList->modelDrawComandCount = 0;
  drawList->meshDrawCommandCount = 0;
  drawList->pointLightCount = 0;
  drawList->directionalLightCount = 0;
  drawList->shadowCastingPointLightCount = 0;
}
