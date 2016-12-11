
#include "debug_renderer.cpp"

static inline void set_uniform(GLint location, S32 value) { glUniform1i(location, value); }
static inline void set_uniform(GLint location, F32 value) { glUniform1f(location, value); }
static inline void set_uniform(GLint location, V2 value) { glUniform2f(location, value.x, value.y); }
static inline void set_uniform(GLint location, V3 value) { glUniform3f(location, value.x, value.y, value.z); }
static inline void set_uniform(GLint location, V4 value) { glUniform4f(location, value.x, value.y, value.z, value.w); }
static inline void set_uniform(GLint location, M4 value) { glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]); }

inline void InitalizeRenderState(RenderState* rs, SystemInfo* sys) {
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
    CreateIndexedVertex1PArray(&rs->skydomeIVA, vertexCount, indexCount, GL_DYNAMIC_DRAW);

    V3 *vertices = 0;
    U32 *indices = 0;
    MapIndexedVertex1PArray(&rs->skydomeIVA, &vertices, &indices, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
    GenerateSubdiviedCubeMeshData(skydomeResolution,
      vertexCount, indexCount,vertices, indices);
    for (U32 i = 0; i < vertexCount; i++)
      vertices[i] = Normalize(vertices[i]);
    UnmapIndexedVertexArray(&rs->skydomeIVA);
  }
    
  initalize_debug_renderer();
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

static inline void BindMaterial(const MaterialDrawable& material) {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, material.diffuse_texture_id);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, material.normal_texture_id);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, material.specular_texture_id);
};

#if 0
static inline
void RenderAtmosphere(const RenderState *rs, const Camera *camera, AssetManifest *am) {
  glDisable(GL_CULL_FACE);
  glUseProgram(GetShaderProgram(ShaderID_Atmosphere, am));
  glBindVertexArray(rs->quadVao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glEnable(GL_CULL_FACE);
}
#endif

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
  glDrawElementsInstanced(GL_TRIANGLES, TERRAIN_INDEX_COUNT_PER_CHUNK, GL_UNSIGNED_INT, 0, TERRAIN_TOTAL_CHUNK_COUNT);
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
};
#endif

static inline void SetLightingUniforms(const VenomDrawList* drawList, const Camera& camera) {
  static const U32 CAMERA_VIEW_POSITON_LOCATION = 4;
  static const U32 DIRECTIONAL_LIGHT_COUNT_LOCATION = 5;
  static const U32 POINT_LIGHT_COUNT_LOCATION = 6;
  static const U32 SHADOW_CASTING_POINT_LIGHT_LOCAION = 7; 

  set_uniform(CAMERA_VIEW_POSITON_LOCATION, camera.position);
  set_uniform(DIRECTIONAL_LIGHT_COUNT_LOCATION, (S32)drawList->directionalLightCount);
  set_uniform(POINT_LIGHT_COUNT_LOCATION, (S32)drawList->pointLightCount);
  set_uniform(SHADOW_CASTING_POINT_LIGHT_LOCAION, (S32)drawList->shadowCastingPointLightCount);

  for (size_t i = 0; i < drawList->directionalLightCount; i++) {
    const DirectionalLight& light = drawList->directionalLights[i];
    set_uniform(DIRECTIONAL_LIGHT_UNIFORM_LOCATION + 0 + (i*UNIFORM_COUNT_PER_DIRECTIONAL_LIGHT), light.direction);
    set_uniform(DIRECTIONAL_LIGHT_UNIFORM_LOCATION + 1 + (i*UNIFORM_COUNT_PER_DIRECTIONAL_LIGHT), light.color);
  }

  for (size_t i = 0; i < drawList->pointLightCount; i++) {
    const PointLight& light = drawList->pointLights[i];
    set_uniform(POINT_LIGHT_UNIFORM_LOCATION + 0 + (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.position);
    set_uniform(POINT_LIGHT_UNIFORM_LOCATION + 1 + (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.color);
    glUniform1f(POINT_LIGHT_UNIFORM_LOCATION + 2 + (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.radius);
  }

  for (size_t i = 0; i < drawList->shadowCastingPointLightCount; i++) {
    const PointLight& light = drawList->shadowCastingPointLights[i];
    set_uniform(SHADOW_CASTING_POINT_LIGHT_UNIFORM_LOCATION + 0 + (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.position);
    set_uniform(SHADOW_CASTING_POINT_LIGHT_UNIFORM_LOCATION + 1 + (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.color);
    set_uniform(SHADOW_CASTING_POINT_LIGHT_UNIFORM_LOCATION + 2 + (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.radius);
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
    ModelDrawable* drawable = GetModelDrawableFromIndex(drawCmd->modelID, manifest);
    //TODO(Torin) This should be an asset condition after default assets are added
    if (drawable == nullptr) continue;

    glUniformMatrix4fv(0, 1, GL_FALSE, &drawCmd->modelMatrix[0][0]);
    U64 currentIndexOffset = 0;
    glBindVertexArray(drawable->vertexArrayID);
    for (U64 j = 0; j < drawable->meshCount; j++) {
      glDrawElements(GL_TRIANGLES, drawable->indexCountPerMesh[j], 
        GL_UNSIGNED_INT, (GLvoid*)(sizeof(U32)*currentIndexOffset));
      currentIndexOffset += drawable->indexCountPerMesh[j];
    }
  }

  for (size_t i = 0; i < drawList->meshDrawCommandCount; i++) {
    VenomMeshDrawCommand* cmd = &drawList->meshDrawCommands[i];
    M4 model = M4Identity();
    glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
    glBindVertexArray(cmd->vertexArrayID);
    glDrawElements(GL_TRIANGLES, cmd->indexCount, GL_UNSIGNED_INT, (GLvoid*)(cmd->indexOffset * sizeof(U32)));
  }

#ifndef VENOM_RELEASE
  auto frameInfo = GetDebugRenderFrameInfo();
  frameInfo->totalDrawListsRendered++;
  frameInfo->totalDrawListCommandsExecuted += drawList->drawCommandCount;
#endif//VENOM_RELEASE
}



//==========================================================================

static inline void draw_atmosphere_oneil(const RenderState *rs, const Camera *camera, AssetManifest *am) {
  static const U32 MVP_MATRIX_LOCATION = 0;
  static const U32 CAMERA_POSITION_LOCATION = 1;
  static const U32 SUN_POSITION_LOCATION = 2;
  glDisable(GL_CULL_FACE);
  M4 mvp_matrix = camera->projection * camera->view * Translate(camera->position);
  V3 lightPosition = { 0.0f, 0.2f, 1.0f };
  glUseProgram(GetShaderProgram(ShaderID_Atmosphere, am));
  glUniformMatrix4fv(MVP_MATRIX_LOCATION, 1, GL_FALSE, &mvp_matrix[0][0]);
  set_uniform(CAMERA_POSITION_LOCATION, camera->position);
  set_uniform(SUN_POSITION_LOCATION, lightPosition);
  glBindVertexArray(rs->skydomeIVA.vertexArrayID);
  glDrawElements(GL_TRIANGLES, rs->skydomeIVA.indexCount, GL_UNSIGNED_INT, 0);
  glEnable(GL_CULL_FACE);
}

static inline void draw_atmosphere_glsl(RenderState *rs, Camera *camera, AssetManifest *assetManifest) {
  static const U32 SUN_POSITION_LOCATION = 0;
  static const U32 CAMERA_POSITION_LOCATION = 1;
  static const U32 CAMERA_DIRECTION_LOCATION = 2;
  static const U32 VIEW_MATRIX_LOCATION = 3;
  V3 sun_position = V3(-1.0f, 0.5f, 0.0f);
  GLuint program_id = GetShaderProgram(ShaderID_atmospheric_scattering_glsl, assetManifest);
  glUseProgram(program_id);
  set_uniform(SUN_POSITION_LOCATION, sun_position);
  M4 mvp = Rotate(-camera->pitch, -camera->yaw, 0.0f) * Translate(0.0f, 0.0f, 1.0f);
  set_uniform(CAMERA_POSITION_LOCATION, camera->position);
  set_uniform(CAMERA_DIRECTION_LOCATION, camera->front);
  set_uniform(VIEW_MATRIX_LOCATION, mvp);
  glBindVertexArray(rs->quadVao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static inline void draw_model_with_materials_common(ModelDrawable *drawable, M4 model_matrix) {
  static const U32 MODEL_MATRIX_LOCATION = 0;
  static const U32 NORMALMAP_PRESENT_LOCATION = 3;
  static const U32 SPECULARMAP_PRESENT_LOCATION = 4;

  static float time = 0.0f;
  Quaternion q = QuaternionFromEulerAngles(0.0f, 0.0f, 10.0f*DEG2RAD*sin(time));
  M4 rotation = QuaternionToMatrix(q);
  time += 0.016f;

  set_uniform(MODEL_MATRIX_LOCATION, model_matrix);
  glBindVertexArray(drawable->vertexArrayID);
  U64 currentIndexOffset = 0;
  for (size_t j = 0; j < drawable->meshCount; j++) {
    const MaterialDrawable &material = drawable->materials[j];
    glUniform1i(NORMALMAP_PRESENT_LOCATION, material.flags & MaterialFlag_NORMAL);
    glUniform1i(SPECULARMAP_PRESENT_LOCATION, material.flags & MaterialFlag_SPECULAR);
    BindMaterial(material);
    glDrawElements(GL_TRIANGLES, drawable->indexCountPerMesh[j], GL_UNSIGNED_INT, (GLvoid*)(sizeof(U32)*currentIndexOffset));
    currentIndexOffset += drawable->indexCountPerMesh[j];
  }
}

static inline void draw_animated_model_with_materials(ModelDrawable *drawable, Animation_State *animation_state, M4 model_matrix) {
  static const U32 BONE_OFFSET_LOCATION = 5;
  static const U32 IS_MESH_STATIC_LOCATION = 21;
  set_uniform(IS_MESH_STATIC_LOCATION, false);
  
  U32 current_index_offset = 0;
  assert(drawable->joint_count < 16);
  M4 local_joint_poses[16];

  Animation_Joint *joints = drawable->joints;
  for (size_t i = 0; i < drawable->joint_count; i++) {
    Animation_Joint *joint = &joints[i];
    local_joint_poses[i] = CalculateLocalJointPose(i, joint, animation_state);
  }

  for (size_t i = 0; i < drawable->joint_count; i++) {
    Animation_Joint *joint = &joints[i];
    M4 global_joint_pose = CalculateGlobalJointPose(i, drawable->joints, local_joint_poses);
    M4 final_skinning_matrix = joint->inverse_bind_matrix * global_joint_pose;
    set_uniform(BONE_OFFSET_LOCATION + i, final_skinning_matrix);
  }

  draw_model_with_materials_common(drawable, model_matrix);
}

static inline void draw_static_model_with_materials(ModelDrawable* drawable, M4 modelMatrix) {
  static const U32 BONE_OFFSET_LOCATION = 5;
  static const U32 IS_MESH_STATIC_LOCATION = 21;
  set_uniform(IS_MESH_STATIC_LOCATION, true);

  for (size_t i = 0; i < 16; i++) {
    set_uniform(BONE_OFFSET_LOCATION + i, M4Identity());
  }

  draw_model_with_materials_common(drawable, modelMatrix);
}

static inline void draw_model_with_only_geometry(ModelDrawable* drawable, M4 modelMatrix) {
  static const U32 MODEL_MATRIX_LOCATION = 0;
  static const U32 BONE_OFFSET_LOCATION = 5;
  U64 currentIndexOffset = 0;
  glBindVertexArray(drawable->vertexArrayID);
  set_uniform(MODEL_MATRIX_LOCATION, modelMatrix);
  for (size_t j = 0; j < drawable->meshCount; j++) {
    glDrawElements(GL_TRIANGLES, drawable->indexCountPerMesh[j], GL_UNSIGNED_INT, (GLvoid*)(sizeof(U32)*currentIndexOffset));
    currentIndexOffset += drawable->indexCountPerMesh[j];
  }
}

//==============================================================================

static inline
void RenderDrawListWithMaterials(VenomDrawList* drawList, AssetManifest* manifest, float deltaTime) {
  for (size_t i = 0; i < drawList->drawCommandCount; i++) {
    VenomDrawCommand* drawCmd = &drawList->drawCommands[i];
    glBindVertexArray(drawCmd->vertexArrayID);
    glDrawElements(GL_TRIANGLES, drawCmd->indexCount, GL_UNSIGNED_INT, 0);
  }

  for (size_t i = 0; i < drawList->animated_model_draw_commands.count; i++) {
    Animated_Model_Draw_Command *draw_cmd = &drawList->animated_model_draw_commands[i];
    ModelDrawable *drawable = &draw_cmd->model->drawable;
    if (drawable == nullptr) continue;
    draw_animated_model_with_materials(drawable, draw_cmd->animation_state, draw_cmd->model_matrix);
  }
    

  for(size_t i = 0; i < drawList->modelDrawComandCount; i++){
    VenomModelDrawCommand* drawCmd = &drawList->modelDrawCommands[i];
    ModelDrawable* drawable = GetModelDrawableFromIndex(drawCmd->modelID, manifest);
    //TODO(Torin) This should be an asset condition after default assets are added
    if (drawable == nullptr) continue;
    draw_static_model_with_materials(drawable, drawCmd->modelMatrix);
  }

  for (size_t i = 0; i < drawList->meshDrawCommandCount; i++) {
    VenomMeshDrawCommand* cmd = &drawList->meshDrawCommands[i];
    const MaterialDrawable& material = GetMaterial(cmd->materialID, manifest);
    BindMaterial(material);
    M4 model = M4Identity();
    glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
    glBindVertexArray(cmd->vertexArrayID);
    glDrawElements(GL_TRIANGLES, cmd->indexCount, GL_UNSIGNED_INT, (GLvoid*)(cmd->indexOffset * sizeof(U32)));
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

static inline void render_outlined_objects(RenderState *rs, Camera *camera, AssetManifest *assetManifest, float deltaTime){
  glEnable(GL_STENCIL_TEST);

  //TODO(Torin) Consolodaite this into a foward render pass procedure
  //instead of duplicating the foward renderer and the outline code

  { //Foward opaque material shader pass
    static const U32 MODEL_MATRIX_LOCATION = 0;
    static const U32 VIEW_MATRIX_LOCATION = 1;
    static const U32 PROJECTION_MATRIX_LOCATION = 2;
    glUseProgram(GetShaderProgram(ShaderID_material_opaque, assetManifest));
    set_uniform(MODEL_MATRIX_LOCATION, M4Identity());
    set_uniform(VIEW_MATRIX_LOCATION, camera->view);
    set_uniform(PROJECTION_MATRIX_LOCATION, camera->projection);
    SetLightingUniforms(&rs->drawList, *camera);
    glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1 << 1, 1 << 1);
    VenomDrawList *drawList = &rs->drawList;
    for (size_t i = 0; i < drawList->outlinedModelDrawCommandCount; i++) {
      VenomModelDrawCommand* drawCmd = &drawList->outlinedModelDrawCommands[i];
      ModelDrawable* drawable = GetModelDrawableFromIndex(drawCmd->modelID, assetManifest);
      draw_static_model_with_materials(drawable, drawCmd->modelMatrix);
    }
  }

  { //Highlight shader pass
    static const U32 VIEW_MATRIX_LOCATION = 1;
    static const U32 PROJECTION_MATRIX_LOCATION = 2;
    static const U32 COLOR_UNIFORM_LOCATION = 3;
    static const V4 OUTLINE_COLOR = { 1.0, 1.0, 0.0, 0.2f };
    glUseProgram(GetShaderProgram(ShaderID_SingleColor, assetManifest));
    set_uniform(VIEW_MATRIX_LOCATION, camera->view);
    set_uniform(PROJECTION_MATRIX_LOCATION, camera->projection);
    set_uniform(COLOR_UNIFORM_LOCATION, OUTLINE_COLOR);
    glStencilFunc(GL_NOTEQUAL, 2, 0xFF);
    glDisable(GL_DEPTH_TEST);
    VenomDrawList *drawList = &rs->drawList;
    for (size_t i = 0; i < drawList->outlinedModelDrawCommandCount; i++) {
      VenomModelDrawCommand* drawCmd = &drawList->outlinedModelDrawCommands[i];
      ModelDrawable* drawable = GetModelDrawableFromIndex(drawCmd->modelID, assetManifest);
      M4 modelMatrix = Translate(drawCmd->position) * Rotate(drawCmd->rotation) * Scale(V3(1.02));
      draw_model_with_only_geometry(drawable, modelMatrix);
    }
    glEnable(GL_DEPTH_TEST);
  }

  glDisable(GL_STENCIL_TEST);
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
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glDisable(GL_SCISSOR_TEST);
  glStencilMask(0xFF);



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
  GLuint opaqueShader = GetShaderProgram(ShaderID_material_opaque, assetManifest);
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
  RenderDrawListWithMaterials(&rs->drawList, assetManifest);
  DebugEnableIf(rs->debugRenderSettings.isWireframeEnabled)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

#else 

#if 1
  { //Geometry pass of the deferred rendering pipeline
    static const U32 MODEL_MATRIX_LOCATION = 0;
    static const U32 VIEW_MATRIX_LOCATION = 1;
    static const U32 PROJECTION_MATRIX_LOCATION = 2;
    GBuffer* gbuffer = &rs->gbuffer;
    glViewport(0, 0, sys->screen_width, sys->screen_height);
    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer->framebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glUseProgram(GetShaderProgram(ShaderID_GBuffer, assetManifest));
    M4 model = M4Identity();
    glUniformMatrix4fv(MODEL_MATRIX_LOCATION, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(VIEW_MATRIX_LOCATION, 1, GL_FALSE, &camera->view[0][0]);
    glUniformMatrix4fv(PROJECTION_MATRIX_LOCATION, 1, GL_FALSE, &camera->projection[0][0]);
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1, 1);
    RenderDrawListWithMaterials(drawList, assetManifest, memory->deltaTime);
    glDisable(GL_STENCIL_TEST);
  }
#endif

#if 0
  { //@SSAO 
    static const U32 SSAO_VIEW_LOCATION = 0;
    static const U32 SSAO_PROJECTION_LOCATION = 1;
    static const U32 SSAO_SAMPLE_LOCATION = 2;
    SSAO* ssao = &rs->ssao;
    GBuffer* gbuffer = &rs->gbuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, ssao->framebuffer);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(GetShaderProgram(ShaderID_SSAO, assetManifest));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer->positionDepth);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gbuffer->normal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ssao->noiseTexture);
    glUniformMatrix4fv(SSAO_VIEW_LOCATION, 1, GL_FALSE, &camera->view[0][0]);
    glUniformMatrix4fv(SSAO_PROJECTION_LOCATION, 1, GL_FALSE, &camera->projection[0][0]);
    for (size_t i = 0; i < SSAO_SAMPLE_COUNT; i++) {
      set_uniform(SSAO_SAMPLE_LOCATION + i, ssao->kernelSamples[i]);
    }
    glBindVertexArray(rs->quadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
#endif

#if 1
  { //Shading pass of defered rendering pipeline
    GBuffer *gbuffer = &rs->gbuffer;
    SSAO *ssao = &rs->ssao;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    static const S32 ZERO_CLEAR_VALUR = 0;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); 
    glClearBufferiv(GL_STENCIL, 0, &ZERO_CLEAR_VALUR);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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

#endif


  { //Copy depth information from the gbuffer into the default framebuffer for foward rendering
    GBuffer *gbuffer = &rs->gbuffer;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer->framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, sys->screen_width, sys->screen_height, 
      0, 0, sys->screen_width, sys->screen_height, 
      GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  //Foward render our debug primatives storing the stencil info
  glEnable(GL_STENCIL_TEST);
  render_debug_draw_commands(camera, assetManifest, memory->deltaTime);
  glDisable(GL_STENCIL_TEST);

  render_outlined_objects(rs, camera, assetManifest, memory->deltaTime);

  //Render the atmosphere first
#if 1
  DEBUG_DisableIf(GetDebugRenderSettings()->disableAtmosphere) {
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0, 0xFF);
    draw_atmosphere_glsl(rs, camera, assetManifest);
    glDisable(GL_STENCIL_TEST);
  }
#endif




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
  drawList->animated_model_draw_commands.count = 0;



#ifndef VENOM_RELEASE
  auto frameInfo = GetDebugRenderFrameInfo();
  frameInfo->pointLightCount = drawList->pointLightCount;
  frameInfo->directionalLightCount = drawList->directionalLightCount;
  frameInfo->shadowCastingPointLightCount = drawList->shadowCastingPointLightCount;
#endif//VENOM_RELEASE
}
