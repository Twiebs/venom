
inline void 
InitalizeRenderState(RenderState* rs, SystemInfo* sys) {
  glGenVertexArrays(1, &rs->quadVao);
  InitGBuffer(&rs->gbuffer, sys->screen_width, sys->screen_height);
  SSAOInit(&rs->ssao, sys->screen_width, sys->screen_height);
  InitCascadedShadowMaps(&rs->csm, sys->screen_width, sys->screen_height, 45.0f*DEG2RAD);
  for (size_t i = 0; i < SHADOW_CASTING_POINT_LIGHT_MAX; i++) {
    InitOmnidirectionalShadowMap(&rs->osm[i]);
  } 


  { //Setup the @skydome
    U32 vertexCount = 0, indexCount = 0;
    static const int skydomeResolution = 8;
    GetSubdiviedCubeVertexAndIndexCount(skydomeResolution, &vertexCount, &indexCount);

    CreateIndexedVertex1PArray(&rs->skydomeIVA, 
      vertexCount, indexCount, GL_DYNAMIC_DRAW);

    V3 *vertices = 0;
    U32 *indices = 0;
    MapIndexedVertex1PArray(&rs->skydomeIVA, &vertices, &indices, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
    GenerateSubdiviedCubeMeshData(skydomeResolution,
      vertexCount, indexCount,vertices, indices);
    for (U32 i = 0; i < vertexCount; i++)
      vertices[i] = Normalize(vertices[i]);
    UnmapIndexedVertexArray(&rs->skydomeIVA);
  }


    
#ifndef VENOM_RELEASE
  CreateDebugRenderResources(&rs->debugRenderResources);
#endif//VENOM_RELEASE
}

static inline
void UpdateCSMFustrums(CascadedShadowMap* csm, Camera* camera) {
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
void RenderAtmosphere(const RenderState *rs, const Camera *camera, AssetManifest *am)
{
  glDisable(GL_CULL_FACE);
  static const U32 mvpLocation = 0;
  static const U32 cameraPositionLocation = 1;
  static const U32 lightPositionLocation = 2;
  M4 mvp = camera->projection * camera->view * Translate(camera->position);
  
  V3 lightPosition = { 0.0f, 1.0f, 0.0f };
  glUseProgram(GetShaderProgram(ShaderID_Atmosphere, am));
  glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, &mvp[0][0]);
  glUniform3fv(cameraPositionLocation, 3, &camera->position.x);
  glUniform3fv(lightPositionLocation, 3, &lightPosition.x);

  glBindVertexArray(rs->skydomeIVA.vao);
  glDrawElements(GL_TRIANGLES, rs->skydomeIVA.indexCount, GL_UNSIGNED_INT, 0);
  glEnable(GL_CULL_FACE);
}

#if 0
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
#endif

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

static inline
void RenderDrawListWithGeometry(VenomDrawList* drawList, AssetManifest* manifest) {
  for (size_t i = 0; i < drawList->drawCommandCount; i++) {
    VenomDrawCommand* drawCmd = &drawList->drawCommands[i];
    glBindVertexArray(drawCmd->vertexArrayID);
    glDrawElements(GL_TRIANGLES, drawCmd->indexCount, GL_UNSIGNED_INT, 0);
  }

  for (size_t i = 0; i < drawList->modelDrawComandCount; i++) {
    VenomModelDrawCommand* drawCmd = &drawList->modelDrawCommands[i];
    const ModelDrawable& drawable = 
      GetModelDrawable(drawCmd->modelID, manifest);
    glUniformMatrix4fv(0, 1, GL_FALSE, &drawCmd->modelMatrix[0][0]);
    U64 currentIndexOffset = 0;
    glBindVertexArray(drawable.vertexArrayID);
    for (U64 j = 0; j < drawable.meshCount; j++) {
      glDrawElements(GL_TRIANGLES, drawable.indexCountPerMesh[j], 
        GL_UNSIGNED_INT, (GLvoid*)(sizeof(U32)*currentIndexOffset));
      currentIndexOffset += drawable.indexCountPerMesh[j];
    }
  }

  for (size_t i = 0; i < drawList->meshDrawCommandCount; i++) {
    VenomMeshDrawCommand* cmd = &drawList->meshDrawCommands[i];
    M4 model = M4Identity();
    glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
    glBindVertexArray(cmd->vertexArrayID);
    glDrawElements(GL_TRIANGLES, cmd->indexCount, 
      GL_UNSIGNED_INT, (GLvoid*)(cmd->indexOffset * sizeof(U32)));
  }

#ifndef VENOM_RELEASE
  auto frameInfo = GetDebugRenderFrameInfo();
  frameInfo->totalDrawListsRendered++;
  frameInfo->totalDrawListCommandsExecuted += drawList->drawCommandCount;
#endif//VENOM_RELEASE
}

static inline
void DrawModelWithMaterialsGBuffer(const ModelDrawable& drawable, const M4& modelMatrix){
  glUniformMatrix4fv(0, 1, GL_FALSE, &modelMatrix[0][0]);
  U64 currentIndexOffset = 0;
  glBindVertexArray(drawable.vertexArrayID);
  for(size_t j = 0; j < drawable.meshCount; j++){
    static const U32 NORMALMAP_PRESENT_LOCATION = 3;
    static const U32 SPECULARMAP_PRESENT_LOCATION = 4;
    const MaterialDrawable &material = drawable.materials[j];
    glUniform1i(NORMALMAP_PRESENT_LOCATION, material.flags & MaterialFlag_NORMAL);
    glUniform1i(SPECULARMAP_PRESENT_LOCATION, material.flags & MaterialFlag_SPECULAR);
    BindMaterial(material);
    glDrawElements(GL_TRIANGLES, drawable.indexCountPerMesh[j], 
      GL_UNSIGNED_INT, (GLvoid*)(sizeof(U32)*currentIndexOffset));
    currentIndexOffset += drawable.indexCountPerMesh[j];
  }
}


static inline
void DrawModelWithMaterialsFoward(const ModelDrawable& drawable, const M4& modelMatrix){
  glUniformMatrix4fv(0, 1, GL_FALSE, &modelMatrix[0][0]);
  U64 currentIndexOffset = 0;
  glBindVertexArray(drawable.vertexArrayID);
  for(size_t j = 0; j < drawable.meshCount; j++){
    static const U32 NORMALMAP_PRESENT_LOCATION = 8;
    static const U32 SPECULARMAP_PRESENT_LOCATION = 999999999;
    const MaterialDrawable &material = drawable.materials[j];
    glUniform1i(NORMALMAP_PRESENT_LOCATION, material.flags & MaterialFlag_NORMAL);
    glUniform1i(SPECULARMAP_PRESENT_LOCATION, material.flags & MaterialFlag_SPECULAR);
    BindMaterial(material);
    glDrawElements(GL_TRIANGLES, drawable.indexCountPerMesh[j], 
      GL_UNSIGNED_INT, (GLvoid*)(sizeof(U32)*currentIndexOffset));
    currentIndexOffset += drawable.indexCountPerMesh[j];
  }
}

static inline
void RenderDrawListWithMaterials(VenomDrawList* drawList, AssetManifest* manifest) {
  for (size_t i = 0; i < drawList->drawCommandCount; i++) {
    VenomDrawCommand* drawCmd = &drawList->drawCommands[i];
    glBindVertexArray(drawCmd->vertexArrayID);
    glDrawElements(GL_TRIANGLES, drawCmd->indexCount, GL_UNSIGNED_INT, 0);
  }

  for(size_t i = 0; i < drawList->modelDrawComandCount; i++){
    VenomModelDrawCommand* drawCmd = &drawList->modelDrawCommands[i];
    const ModelDrawable& drawable = 
      GetModelDrawable(drawCmd->modelID, manifest);
    DrawModelWithMaterialsGBuffer(drawable, drawCmd->modelMatrix);
  }

  for (size_t i = 0; i < drawList->meshDrawCommandCount; i++) {
    VenomMeshDrawCommand* cmd = &drawList->meshDrawCommands[i];
    const MaterialDrawable& material = GetMaterial(cmd->materialID, manifest);
    BindMaterial(material);
    M4 model = M4Identity();
    glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
    glBindVertexArray(cmd->vertexArrayID);
    glDrawElements(GL_TRIANGLES, cmd->indexCount, 
      GL_UNSIGNED_INT, (GLvoid*)(cmd->indexOffset * sizeof(U32)));
  }

#ifndef VENOM_RELEASE
  auto frameInfo = GetDebugRenderFrameInfo();
  frameInfo->totalDrawListsRendered++;
  frameInfo->totalDrawListCommandsExecuted += drawList->drawCommandCount;
#endif//VENOM_RELEASE
}

static inline
void RenderCSM(CascadedShadowMap* csm, Camera* camera, 
  DirectionalLight* light, VenomDrawList* drawList, AssetManifest* manifest) 
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


    static const U32 lightSpaceTransformLocation = 3;
		glViewport(0, 0, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, 
      GL_DEPTH_ATTACHMENT, csm->depthTexture, 0, i);
		glClear(GL_DEPTH_BUFFER_BIT);
		glUniformMatrix4fv(lightSpaceTransformLocation, 1, 0, &light_transform[0][0]);
    RenderDrawListWithGeometry(drawList, manifest);
	}
}

#ifndef VENOM_RELEASE
#define DebugEnableIf(expr) if (expr)
#define DebugDisableIf(expr) if(!expr)
#define DEBUG_DisableIf(expr) if(!expr)
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
  AssetManifest* assetManifest = &memory->assetManifest;
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

#if 1
  {
    CascadedShadowMap* csm = &rs->csm;
    
    glViewport(0, 0, OMNI_SHADOW_MAP_RESOLUTION, OMNI_SHADOW_MAP_RESOLUTION);
    glBindFramebuffer(GL_FRAMEBUFFER, csm->framebuffer);
    GLuint programID = GetShaderProgram(ShaderID_depth_cubemap, assetManifest);
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
      RenderDrawListWithGeometry(&rs->drawList, assetManifest);
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

#if 0
  //NOTE(Torin) Foward Render Pass
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
  RenderDrawListWithMaterials(&rs->drawList, &memory->assets);
  DebugEnableIf(rs->debugRenderSettings.isWireframeEnabled) 
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

#else 
  //NOTE(Torin) Defered Render Pass
  GBuffer* gbuffer = &rs->gbuffer;
  SSAO* ssao = &rs->ssao;

  static const U32 modelMatrixLocation = 0;
  static const U32 viewMatrixLocation = 1;
  static const U32 projectionMatrixLocation = 2;

  { //@GBuffer 
    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer->framebuffer);
    glViewport(0, 0, sys->screen_width, sys->screen_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glUseProgram(GetShaderProgram(ShaderID_GBuffer, assetManifest));
    M4 model = M4Identity();
    glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(1, 1, GL_FALSE, &camera->view[0][0]);
    glUniformMatrix4fv(2, 1, GL_FALSE, &camera->projection[0][0]);
    RenderDrawListWithMaterials(drawList, assetManifest);
  }

  
#if 0
  { //@SSAO 
    glBindFramebuffer(GL_FRAMEBUFFER, ssao->framebuffer);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(GetShaderProgram(ShaderID_SSAO, assetManifest));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer->positionDepth);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gbuffer->normal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ssao->noiseTexture);
    static const U32 SSAO_VIEW_LOCATION = 0;
    static const U32 SSAO_PROJECTION_LOCATION = 1;
    static const U32 SSAO_SAMPLE_LOCATION = 2;
    glUniformMatrix4fv(SSAO_VIEW_LOCATION, 
      1, GL_FALSE, &camera->view[0][0]);
    glUniformMatrix4fv(SSAO_PROJECTION_LOCATION, 
      1, GL_FALSE, &camera->projection[0][0]);
    for (size_t i = 0; i < SSAO_SAMPLE_COUNT; i++) {
      glUniform3fv(SSAO_SAMPLE_LOCATION + i, 1, &ssao->kernelSamples[i].x);
    }
    glBindVertexArray(rs->quadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
#endif

  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    glUseProgram(GetShaderProgram(ShaderID_DeferredMaterial, assetManifest));
    SetLightingUniforms(drawList, *camera);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer->positionDepth);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gbuffer->normal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gbuffer->albedoSpecular);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, ssao->bufferTexture);
    glBindVertexArray(rs->quadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }

#endif

  glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer->framebuffer);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, sys->screen_width, sys->screen_height,
    0, 0, sys->screen_width, sys->screen_height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  DEBUG_DisableIf(GetDebugRenderSettings()->disableAtmosphere) {
    RenderAtmosphere(rs, camera, assetManifest);
  }


  glUseProgram(GetShaderProgram(ShaderID_material_opaque, assetManifest));
  glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &camera->view[0][0]);
  glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &camera->projection[0][0]);
  SetLightingUniforms(&rs->drawList, *camera);
  {
    //NOTE(Torin) Draw Outlined Objects
    const S32 ZERO = 0; 
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glClearBufferiv(GL_STENCIL, 0, &ZERO);
    glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
    glStencilMask(0xFF);
    
#if 1
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    for(size_t i = 0; i < drawList->outlinedModelDrawCommandCount; i++){
      VenomModelDrawCommand* drawCmd = &drawList->outlinedModelDrawCommands[i];
      const ModelDrawable& drawable = 
        GetModelDrawable(drawCmd->modelID, assetManifest);
      DrawModelWithMaterialsFoward(drawable, drawCmd->modelMatrix);
    }
#endif

#if 1
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilMask(0x00);
    glDisable(GL_DEPTH_TEST);
    static const U32 colorUniformLocation = 3;
    const V4 outlineColor = { 1.0, 0.0, 1.0, 1.0 };
    glUseProgram(GetShaderProgram(ShaderID_SingleColor, assetManifest));
    glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &camera->view[0][0]);
    glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &camera->projection[0][0]);
    glUniform4fv(colorUniformLocation, 1, &outlineColor.x);
    
    for(size_t i = 0; i < drawList->outlinedModelDrawCommandCount; i++){
      VenomModelDrawCommand* drawCmd = &drawList->outlinedModelDrawCommands[i];
      const ModelDrawable& drawable = 
        GetModelDrawable(drawCmd->modelID, assetManifest);
      M4 model = Translate(drawCmd->position) * 
        Rotate(drawCmd->rotation) * Scale(V3(1.1));
      DrawModelWithMaterialsFoward(drawable, model);
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
#endif
  }

  //@Physics @Colliders @Draw
  DebugEnableIf(GetDebugRenderSettings()->drawPhysicsColliders){ 
    const DebugRenderResources& debugResources = rs->debugRenderResources;
    const U32 modelMatrixLocation = 0;
    const U32 viewMatrixLocation = 1;
    const U32 projectionMatrixLocation = 2;
    const U32 colorLocation = 3;
    //TODO(Torin) Quick and diry hack for line drawing
    const U32 lineSegmentPositionsLocation = 4;
    const U32 useLinePosLocation = 6;

    M4 modelMatrix = M4Identity();
    glUseProgram(GetShaderProgram(ShaderID_DebugShape, assetManifest));
    glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]); 
    glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &camera->view[0][0]);
    glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &camera->projection[0][0]);
    glBindVertexArray(debugResources.vao);
    for(size_t i = 0; i < drawList->debugCommandCount; i++) {
      DebugRenderCommand* cmd = &drawList->debugCommands[i];
      if(cmd->isSolid == false) { 
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      }

      glUniform4fv(colorLocation, 1, &cmd->color.x);
      switch(cmd->type) {
        case DebugRenderCommandType_Box: {
          V3 boundsSize = Abs(cmd->max - cmd->min);
          modelMatrix = Translate(cmd->min + (boundsSize * 0.5f)) * Scale(boundsSize); 
          glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]); 
          glDrawElements(GL_TRIANGLES, debugResources.cubeIndexCount, GL_UNSIGNED_INT, (void *)(uintptr_t)(debugResources.cubeIndexOffset));
        } break;

        case DebugRenderCommandType_Sphere: {
          modelMatrix = Translate(cmd->center) * Scale(cmd->radius);
          glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]); 
          glDrawElements(GL_TRIANGLES, debugResources.sphereIndexCount, 
            GL_UNSIGNED_INT, (void*)(uintptr_t)debugResources.sphereIndexOffset);
        } break;

        case DebugRenderCommandType_Line: {
          glUniform1i(useLinePosLocation, 1);
          modelMatrix = M4Identity();
          glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]); 
          glUniform3fv(lineSegmentPositionsLocation, 2, (float*)cmd->lineSegmentPositions);
          glDrawArrays(GL_LINES, 0, 2);
          glUniform1i(useLinePosLocation, 0);
        } break;
        
        case DebugRenderCommandType_Axis: {
          M4 translation = Translate(cmd->position);
          glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &translation[0][0]); 
          glUniform4f(colorLocation, 1.0, 0.0, 0.0, 1.0);
          glDrawElements(GL_TRIANGLES, debugResources.axisIndexCount,
            GL_UNSIGNED_INT, (void*)(uintptr_t)debugResources.axisIndexOffset);
          
          glUniform4f(colorLocation, 0.0, 1.0, 0.0, 1.0);
          M4 model = translation * Rotate(0.0, 0.0, -DEG2RAD*90.0f);
          glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &model[0][0]); 
          glDrawElements(GL_TRIANGLES, debugResources.axisIndexCount,
            GL_UNSIGNED_INT, (void*)(uintptr_t)debugResources.axisIndexOffset);

          glUniform4f(colorLocation, 0.0, 0.0, 1.0, 1.0);
          model = translation * Rotate(0.0, DEG2RAD*90.0f, 0.0f);
          glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &model[0][0]); 
          glDrawElements(GL_TRIANGLES, debugResources.axisIndexCount,
            GL_UNSIGNED_INT, (void*)(uintptr_t)debugResources.axisIndexOffset);

        } break;

      }

      cmd->duration -= memory->deltaTime;
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glEnable(GL_CULL_FACE);
    }
  }


#if 0

  DebugEnableIf(rs->debugRenderSettings.renderDebugNormals) {
    GLuint shader = GetShaderProgram(ShaderID_debug_normals, assets);
    glUseProgram(shader);
    glUniformMatrix4fv(1, 1, GL_FALSE, &camera->view[0][0]);
    glUniformMatrix4fv(2, 1, GL_FALSE, &camera->projection[0][0]);
    RenderDrawListWithGeometry(&rs->drawList, assets);
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

  drawList->drawCommandCount = 0;
  drawList->modelDrawComandCount = 0;
  drawList->meshDrawCommandCount = 0;
  drawList->outlinedModelDrawCommandCount = 0;
  drawList->pointLightCount = 0;
  drawList->directionalLightCount = 0;
  drawList->shadowCastingPointLightCount = 0;

  size_t newDebugCommandCount = 0;
  for(size_t i = 0; i < drawList->debugCommandCount; i++){
    if(drawList->debugCommands[i].duration > 0) {
      drawList->debugCommands[newDebugCommandCount] = drawList->debugCommands[i];
      newDebugCommandCount++;
    }
  }
  drawList->debugCommandCount = newDebugCommandCount;

#ifndef VENOM_RELEASE
  auto frameInfo = GetDebugRenderFrameInfo();
  frameInfo->pointLightCount = drawList->pointLightCount;
  frameInfo->directionalLightCount = drawList->directionalLightCount;
  frameInfo->shadowCastingPointLightCount = drawList->shadowCastingPointLightCount;
#endif//VENOM_RELEASE
}
