#include "debug_renderer.cpp"
#include "Render/DebugRenderPass.cpp"

inline void SetUniform(GLint location, S32 value) { glUniform1i(location, value); }
inline void SetUniform(GLint location, F32 value) { glUniform1f(location, value); }
inline void SetUniform(GLint location, V2 value)  { glUniform2f(location, value.x, value.y); }
inline void SetUniform(GLint location, V3 value)  { glUniform3f(location, value.x, value.y, value.z); }
inline void SetUniform(GLint location, V4 value)  { glUniform4f(location, value.x, value.y, value.z, value.w); }
inline void SetUniform(GLint location, M4 value)  { glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]); }

static inline void BindMaterial(MaterialData *material) {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, material->diffuseTextureID);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, material->normalTextureID);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, material->specularTextureID);
};

static inline void SetLightingUniforms(const VenomDrawList* drawList, const Camera& camera) {
  static const U32 CAMERA_VIEW_POSITON_LOCATION = 4;
  static const U32 DIRECTIONAL_LIGHT_COUNT_LOCATION = 5;
  static const U32 POINT_LIGHT_COUNT_LOCATION = 6;
  static const U32 SHADOW_CASTING_POINT_LIGHT_LOCAION = 7;

  SetUniform(CAMERA_VIEW_POSITON_LOCATION, camera.position);
  SetUniform(DIRECTIONAL_LIGHT_COUNT_LOCATION, (S32)drawList->directionalLightCount);
  SetUniform(POINT_LIGHT_COUNT_LOCATION, (S32)drawList->pointLightCount);
  SetUniform(SHADOW_CASTING_POINT_LIGHT_LOCAION, (S32)drawList->shadowCastingPointLightCount);

  for (size_t i = 0; i < drawList->directionalLightCount; i++) {
    const DirectionalLight& light = drawList->directionalLights[i];
    SetUniform(DIRECTIONAL_LIGHT_UNIFORM_LOCATION + 0 + (i*UNIFORM_COUNT_PER_DIRECTIONAL_LIGHT), light.direction);
    SetUniform(DIRECTIONAL_LIGHT_UNIFORM_LOCATION + 1 + (i*UNIFORM_COUNT_PER_DIRECTIONAL_LIGHT), light.color);
  }

  for (size_t i = 0; i < drawList->pointLightCount; i++) {
    const PointLight& light = drawList->pointLights[i];
    SetUniform(POINT_LIGHT_UNIFORM_LOCATION + 0 + (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.position);
    SetUniform(POINT_LIGHT_UNIFORM_LOCATION + 1 + (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.color);
    glUniform1f(POINT_LIGHT_UNIFORM_LOCATION + 2 + (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.radius);
  }

  for (size_t i = 0; i < drawList->shadowCastingPointLightCount; i++) {
    const PointLight& light = drawList->shadowCastingPointLights[i];
    SetUniform(SHADOW_CASTING_POINT_LIGHT_UNIFORM_LOCATION + 0 + (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.position);
    SetUniform(SHADOW_CASTING_POINT_LIGHT_UNIFORM_LOCATION + 1 + (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.color);
    SetUniform(SHADOW_CASTING_POINT_LIGHT_UNIFORM_LOCATION + 2 + (i*UNIFORM_COUNT_PER_POINT_LIGHT), light.radius);
  }
}

static inline void SetLightSpaceTransformUniforms(CascadedShadowMap* csm, GLuint programID) {
  GLint matrixLocation = glGetUniformLocation(programID, "u_light_space_matrix");
  GLint distanceLocation = glGetUniformLocation(programID, "u_shadow_cascade_distance");
  glUniformMatrix4fv(matrixLocation, SHADOW_MAP_CASCADE_COUNT, GL_FALSE, &csm->lightSpaceTransforms[0][0][0]);
  glUniform1fv(distanceLocation, SHADOW_MAP_CASCADE_COUNT, &csm->shadowCascadeDistances[0]);
}

//=========================================================================================================

#if 1
static inline void DrawTerrainGeometry(TerrainGenerationState *terrainGenState) {
  glActiveTexture(GL_TEXTURE5);
  glBindTexture(GL_TEXTURE_2D_ARRAY, terrainGenState->heightmap_texture_array);
  glActiveTexture(GL_TEXTURE6);
  glBindTexture(GL_TEXTURE_2D_ARRAY, terrainGenState->normals_texture_array);
  glActiveTexture(GL_TEXTURE7);
  glBindTexture(GL_TEXTURE_2D_ARRAY, terrainGenState->detailmap_texture_array);
  glBindVertexArray(terrainGenState->base_mesh.vertexArrayID);
  glDrawElementsInstanced(GL_TRIANGLES, TerrainParameters::INDEX_COUNT_PER_CHUNK, GL_UNSIGNED_INT, 0, TerrainParameters::CHUNK_COUNT);
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
};

static inline void DrawTerrain(RenderState *rs, TerrainGenerationState *terrain, Camera *camera) {
  static const S32 MODEL_MATRIX_LOCATION = 0;
  static const S32 VIEW_MATRIX_LOCATION = 1;
  static const S32 PROJECTION_MATRIX_LOCATION = 2;

  AssetManifest *assets = GetAssetManifest();
  GLuint shaderProgram = GetShaderProgram(ShaderID_terrain, assets);
  glUseProgram(shaderProgram);
  SetLightingUniforms(&rs->drawList, *camera);
  SetUniform(MODEL_MATRIX_LOCATION, M4Identity());
  SetUniform(VIEW_MATRIX_LOCATION, camera->viewMatrix);
  SetUniform(PROJECTION_MATRIX_LOCATION, camera->projectionMatrix);
  DrawTerrainGeometry(terrain);

  //TODO(Torin) Give the terrain a better material system
  //auto& material0 = GetMaterial(&memory->assets, MaterialID_dirt00);
  //auto& material1 = GetMaterial(&memory->assets, MaterialID_grass01);
  //glActiveTexture(GL_TEXTURE0);
  //glBindTexture(GL_TEXTURE_2D, material0.diffuse_texture_id);
  //glActiveTexture(GL_TEXTURE1);
  //glBindTexture(GL_TEXTURE_2D, material1.diffuse_texture_id);
}
#endif

static inline void DrawTerrain(RenderState *rs, Terrain *terrain, Camera *camera) {
  static const S32 MVP_MATRIX_LOCATION = 0;
  const M4 mvpMatrix = camera->projectionMatrix * camera->viewMatrix;
  glUseProgram(GetShaderProgram(ShaderID_NewTerrain));
  SetUniform(MVP_MATRIX_LOCATION, mvpMatrix);
  SetLightingUniforms(&rs->drawList, *camera);
  for (size_t i = 0; i < terrain->chunkCount; i++) {
    TerrainChunk *chunk = &terrain->chunks[i];
    glBindVertexArray(chunk->vertexArrayID);
    glDrawArrays(GL_TRIANGLES, 0, terrain->vertexCountPerChunk);
  }
}
static inline void DrawAtmosphereOneil(const RenderState *rs, const Camera *camera) {
  static const U32 MVP_MATRIX_LOCATION = 0;
  static const U32 CAMERA_POSITION_LOCATION = 1;
  static const U32 SUN_POSITION_LOCATION = 2;
  glDisable(GL_CULL_FACE);
  M4 mvp_matrix = camera->projectionMatrix * camera->viewMatrix * Translate(camera->position);
  V3 lightPosition = { 0.0f, 0.2f, 1.0f };
  AssetManifest *am = GetAssetManifest();
  glUseProgram(GetShaderProgram(ShaderID_Atmosphere, am));
  glUniformMatrix4fv(MVP_MATRIX_LOCATION, 1, GL_FALSE, &mvp_matrix[0][0]);
  SetUniform(CAMERA_POSITION_LOCATION, camera->position);
  SetUniform(SUN_POSITION_LOCATION, lightPosition);
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
  SetUniform(SUN_POSITION_LOCATION, sun_position);
  M4 mvp = camera->viewMatrix * Translate(0.0f, 0.0f, 1.0f);
  V3 cameraDirection = V3(camera->viewMatrix * V4(0.0f, 0.0f, -1.0f, 0.0f));
  SetUniform(CAMERA_POSITION_LOCATION, camera->position);
  SetUniform(CAMERA_DIRECTION_LOCATION, cameraDirection);
  SetUniform(VIEW_MATRIX_LOCATION, mvp);
  glBindVertexArray(rs->quadVao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static inline void DrawModelWithMaterialsCommon(ModelAsset *model, M4 modelMatrix) {
  static const U32 MODEL_MATRIX_LOCATION = 0;
  static const U32 NORMALMAP_PRESENT_LOCATION = 3;
  static const U32 SPECULARMAP_PRESENT_LOCATION = 4;

  SetUniform(MODEL_MATRIX_LOCATION, modelMatrix);
  glBindVertexArray(model->vertexArray.vertexArrayID);
  U64 currentIndexOffset = 0;
  for (size_t j = 0; j < model->meshCount; j++) {
    MaterialData *material = &model->materialDataPerMesh[j];
    glUniform1i(NORMALMAP_PRESENT_LOCATION, material->materialFlags & MaterialFlag_NORMAL);
    glUniform1i(SPECULARMAP_PRESENT_LOCATION, material->materialFlags & MaterialFlag_SPECULAR);
    BindMaterial(material);
    glDrawElements(GL_TRIANGLES, model->indexCountPerMesh[j], GL_UNSIGNED_INT, (GLvoid*)(sizeof(U32)*currentIndexOffset));
    currentIndexOffset += model->indexCountPerMesh[j];
  }
}

static inline void DrawAnimatedModelWithMaterials(ModelAsset *model, AnimationState *animation_state, M4 model_matrix) {
  static const U32 BONE_OFFSET_LOCATION = 5;
  static const U32 IS_MESH_STATIC_LOCATION = 69;
  SetUniform(IS_MESH_STATIC_LOCATION, false);

  Animation_Joint *joints = model->joints;

#if 0
  M4 local_joint_poses[64];

  for (size_t i = 0; i < drawable->joint_count; i++) {
    Animation_Joint *joint = &joints[i];
    local_joint_poses[i] = CalculateLocalJointPose(i, joint, animation_state);
  }

  for (size_t i = 0; i < drawable->joint_count; i++) {
    Animation_Joint *joint = &join
      ts[i];
    M4 global_joint_pose = CalculateGlobalJointPose(i, drawable->joints, local_joint_poses);
    M4 final_skinning_matrix = CalculateSkinningMatrix(joint, global_joint_pose);
    SetUniform(BONE_OFFSET_LOCATION + i, final_skinning_matrix);
  }
#endif

  auto globalPoses = (M4 *)Memory::FrameStackPointer(animation_state->globalPoseOffset);
  for (size_t i = 0; i < model->jointCount; i++) {
    Animation_Joint *joint = &joints[i];
    M4 final_skinning_matrix = CalculateSkinningMatrix(joint, globalPoses[i]);
    SetUniform(BONE_OFFSET_LOCATION + i, final_skinning_matrix);
  }

  DrawModelWithMaterialsCommon(model, model_matrix);
}

static inline void DrawStaticModelWithMaterials(ModelAsset* model, M4 modelMatrix) {
  static const U32 BONE_OFFSET_LOCATION = 5;
  static const U32 IS_MESH_STATIC_LOCATION = 69;
  SetUniform(IS_MESH_STATIC_LOCATION, true);
  for (size_t i = 0; i < 16; i++) {
    SetUniform(BONE_OFFSET_LOCATION + i, M4Identity());
  }

  DrawModelWithMaterialsCommon(model, modelMatrix);
}

static inline void DrawModelWithOnlyGeometry(ModelAsset* model, M4 modelMatrix) {
  static const U32 MODEL_MATRIX_LOCATION = 0;
  static const U32 BONE_OFFSET_LOCATION = 5;
  U64 currentIndexOffset = 0;
  glBindVertexArray(model->vertexArray.vertexArrayID);
  SetUniform(MODEL_MATRIX_LOCATION, modelMatrix);
  for (size_t j = 0; j < model->meshCount; j++) {
    glDrawElements(GL_TRIANGLES, model->indexCountPerMesh[j], GL_UNSIGNED_INT, (GLvoid*)(sizeof(U32)*currentIndexOffset));
    currentIndexOffset += model->indexCountPerMesh[j];
  }
}

//============================================================================================================

static inline void RenderDrawListWithGeometry(VenomDrawList* drawList, AssetManifest* manifest) {
  for (size_t i = 0; i < drawList->drawCommandCount; i++) {
    VenomDrawCommand* drawCmd = &drawList->drawCommands[i];
    glBindVertexArray(drawCmd->vertexArrayID);
    glDrawElements(GL_TRIANGLES, drawCmd->indexCount, GL_UNSIGNED_INT, 0);
  }

  for (size_t i = 0; i < drawList->modelDrawComandCount; i++) {
    VenomModelDrawCommand* drawCmd = &drawList->modelDrawCommands[i];
    ModelAsset *model = drawCmd->model;
    assert(model != nullptr);

    glUniformMatrix4fv(0, 1, GL_FALSE, &drawCmd->modelMatrix[0][0]);
    U64 currentIndexOffset = 0;
    glBindVertexArray(model->vertexArray.vertexArrayID);
    for (U64 j = 0; j < model->meshCount; j++) {
      glDrawElements(GL_TRIANGLES, model->indexCountPerMesh[j],
        GL_UNSIGNED_INT, (GLvoid*)(sizeof(U32)*currentIndexOffset));
      currentIndexOffset += model->indexCountPerMesh[j];
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

static inline void RenderDrawListWithMaterials(VenomDrawList* drawList, AssetManifest* manifest, float deltaTime) {
  for (size_t i = 0; i < drawList->drawCommandCount; i++) {
    VenomDrawCommand* drawCmd = &drawList->drawCommands[i];
    glBindVertexArray(drawCmd->vertexArrayID);
    glDrawElements(GL_TRIANGLES, drawCmd->indexCount, GL_UNSIGNED_INT, 0);
  }

  for (size_t i = 0; i < drawList->animated_model_draw_commands.count; i++) {
    Animated_Model_Draw_Command *draw_cmd = &drawList->animated_model_draw_commands[i];
    ModelAsset *model = draw_cmd->model;
    assert(model != nullptr);
    DrawAnimatedModelWithMaterials(model, draw_cmd->animation_state, draw_cmd->model_matrix);
  }


  for (size_t i = 0; i < drawList->modelDrawComandCount; i++) {
    VenomModelDrawCommand* drawCmd = &drawList->modelDrawCommands[i];
    ModelAsset *model = drawCmd->model;
    assert(model != nullptr);
    DrawStaticModelWithMaterials(model, drawCmd->modelMatrix);
  }

#if 0
  for (size_t i = 0; i < drawList->meshDrawCommandCount; i++) {
    VenomMeshDrawCommand* cmd = &drawList->meshDrawCommands[i];
    const MaterialDrawable& material = GetMaterial(cmd->materialID, manifest);
    BindMaterial(material);
    M4 model = M4Identity();
    glUniformMatrix4fv(0, 1, GL_FALSE, &model[0][0]);
    glBindVertexArray(cmd->vertexArrayID);
    glDrawElements(GL_TRIANGLES, cmd->indexCount, GL_UNSIGNED_INT, (GLvoid*)(cmd->indexOffset * sizeof(U32)));
  }
#endif

#ifndef VENOM_RELEASE
  auto frameInfo = GetDebugRenderFrameInfo();
  frameInfo->totalDrawListsRendered++;
  frameInfo->totalDrawListCommandsExecuted += drawList->drawCommandCount;
#endif//VENOM_RELEASE
}


//===========================================================================================================

inline void InitalizeRenderState(RenderState* rs, SystemInfo* sys) {
  glGenVertexArrays(1, &rs->quadVao);
  InitGBuffer(&rs->gbuffer, sys->screenWidth, sys->screenHeight);
  SSAOInit(&rs->ssao, sys->screenWidth, sys->screenHeight);
  InitCascadedShadowMaps(&rs->csm, sys->screenWidth, sys->screenHeight, 45.0f*DEG2RAD);
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

static inline void UpdateCSMFustrums(CascadedShadowMap* csm, Camera* camera) {
  Frustum *f = csm->cascadeFrustums;
  float clip_plane_distance_ratio = camera->farClip / camera->nearClip;
  f[0].near_plane_distance = camera->nearClip;
  for (U32 i = 1; i < SHADOW_MAP_CASCADE_COUNT; i++) {
    float scalar = (float)i / (float)SHADOW_MAP_CASCADE_COUNT;
    f[i].near_plane_distance = SHADOW_MAP_CASCADE_WEIGHT *
      (camera->nearClip * powf(clip_plane_distance_ratio, scalar)) +
      (1 - SHADOW_MAP_CASCADE_WEIGHT) * (camera->nearClip +
      (camera->farClip - camera->nearClip) * scalar);
    f[i - 1].far_plane_distance = f[i].near_plane_distance * SHADOW_MAP_CASCADE_TOLERANCE;
  }
  f[SHADOW_MAP_CASCADE_COUNT - 1].far_plane_distance = camera->farClip;
}

static inline void RenderCSM(CascadedShadowMap* csm, Camera* camera, DirectionalLight* light, VenomDrawList* drawList, AssetManifest* manifest) {
  M4 light_view = LookAt(V3(0.0f), -light->direction, V3(-1.0f, 0.0f, 0.0f));
  for (U32 i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
    //Compute cascade frustum points	
    Frustum *f = csm->cascadeFrustums + i;
    csm->shadowCascadeDistances[i] = 0.5f * (-f->far_plane_distance *
      camera->projectionMatrix[2][2] + camera->projectionMatrix[3][2]) /
      f->far_plane_distance + 0.5f;

    V3 up = V3(0.0f, 1.0f, 0.0f);
    V3 view_direction = V3(camera->viewMatrix * V4(0.0f, 0.0f, -1.0f, 0.0f));
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

static inline void render_outlined_objects(RenderState *rs, Camera *camera, AssetManifest *assetManifest, float deltaTime){
  glEnable(GL_STENCIL_TEST);

  //TODO(Torin) Consolodaite this into a foward render pass procedure
  //instead of duplicating the foward renderer and the outline code

  { //Foward opaque material shader pass
    static const U32 MODEL_MATRIX_LOCATION = 0;
    static const U32 VIEW_MATRIX_LOCATION = 1;
    static const U32 PROJECTION_MATRIX_LOCATION = 2;
    glUseProgram(GetShaderProgram(ShaderID_material_opaque, assetManifest));
    SetUniform(MODEL_MATRIX_LOCATION, M4Identity());
    SetUniform(VIEW_MATRIX_LOCATION, camera->viewMatrix);
    SetUniform(PROJECTION_MATRIX_LOCATION, camera->projectionMatrix);
    SetLightingUniforms(&rs->drawList, *camera);
    glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1 << 1, 1 << 1);
    VenomDrawList *drawList = &rs->drawList;
    for (size_t i = 0; i < drawList->outlinedModelDrawCommandCount; i++) {
      VenomModelDrawCommand* drawCmd = &drawList->outlinedModelDrawCommands[i];
      ModelAsset *model = drawCmd->model;
      DrawStaticModelWithMaterials(model, drawCmd->modelMatrix);
    }
  }

  { //Highlight shader pass
    static const U32 VIEW_MATRIX_LOCATION = 1;
    static const U32 PROJECTION_MATRIX_LOCATION = 2;
    static const U32 COLOR_UNIFORM_LOCATION = 3;
    static const V4 OUTLINE_COLOR = { 1.0, 1.0, 0.0, 0.2f };
    glUseProgram(GetShaderProgram(ShaderID_SingleColor, assetManifest));
    SetUniform(VIEW_MATRIX_LOCATION, camera->viewMatrix);
    SetUniform(PROJECTION_MATRIX_LOCATION, camera->projectionMatrix);
    SetUniform(COLOR_UNIFORM_LOCATION, OUTLINE_COLOR);
    glStencilFunc(GL_NOTEQUAL, 2, 0xFF);
    glDisable(GL_DEPTH_TEST);
    VenomDrawList *drawList = &rs->drawList;
    for (size_t i = 0; i < drawList->outlinedModelDrawCommandCount; i++) {
      VenomModelDrawCommand* drawCmd = &drawList->outlinedModelDrawCommands[i];
      ModelAsset *model = drawCmd->model;
      assert(model != nullptr);
      M4 modelMatrix = Translate(drawCmd->position) * Rotate(drawCmd->rotation) * Scale(V3(1.02));
      DrawModelWithOnlyGeometry(model, modelMatrix);
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
  glViewport(0, 0, sys->screenWidth, sys->screenHeight);
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
    glViewport(0, 0, sys->screenWidth, sys->screenHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer->framebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glUseProgram(GetShaderProgram(ShaderID_GBuffer, assetManifest));
    M4 model = M4Identity();
    glUniformMatrix4fv(MODEL_MATRIX_LOCATION, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(VIEW_MATRIX_LOCATION, 1, GL_FALSE, &camera->viewMatrix[0][0]);
    glUniformMatrix4fv(PROJECTION_MATRIX_LOCATION, 1, GL_FALSE, &camera->projectionMatrix[0][0]);
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
      SetUniform(SSAO_SAMPLE_LOCATION + i, ssao->kernelSamples[i]);
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
    glBlitFramebuffer(0, 0, sys->screenWidth, sys->screenHeight, 
      0, 0, sys->screenWidth, sys->screenHeight, 
      GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  VenomDebugRenderSettings *settings = GetDebugRenderSettings();

  //Foward render our debug primatives storing the stencil info
  glEnable(GL_STENCIL_TEST);
  if (rs->terrain != nullptr) {
    if (settings->isWireframeEnabled) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    //DrawTerrain(rs, rs->terrain, camera);
    DrawTerrain(rs, rs->newTerrain, camera);

    if (settings->isWireframeEnabled) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
  }

  render_debug_draw_commands(camera, assetManifest, memory->deltaTime);
  DebugRenderPass(rs);
  glDisable(GL_STENCIL_TEST);

  render_outlined_objects(rs, camera, assetManifest, memory->deltaTime);

  DEBUG_DisableIf(GetDebugRenderSettings()->disableAtmosphere) {
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0, 0xFF);
    DrawAtmosphereOneil(rs, camera);
    glDisable(GL_STENCIL_TEST);
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
  drawList->animated_model_draw_commands.count = 0;

#ifndef VENOM_RELEASE
  auto frameInfo = GetDebugRenderFrameInfo();
  frameInfo->pointLightCount = drawList->pointLightCount;
  frameInfo->directionalLightCount = drawList->directionalLightCount;
  frameInfo->shadowCastingPointLightCount = drawList->shadowCastingPointLightCount;
#endif//VENOM_RELEASE
}
