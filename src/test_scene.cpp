#define VENOM_ENTITY_METALIST_MACRO EntityTypeList
#define VENOM_ENTITY_STRUCT Entity
#define VENOM_MATERIAL_LIST_FILE "asset_list.h"

#include "venom_module.cpp"
#include "venom_editor.cpp"
#include "venom_entity.cpp"

struct GameData {
  Camera camera;
  EntityContainer entityContainer;
  EditorData editorData;
  bool initalized;
  Player player;
  IndexedVertexArray proceduralMesh;
};

void VenomModuleStart(GameMemory* memory) {
  SystemInfo* sys = &memory->systemInfo;
  GameData* data = PushStruct(GameData, &memory->mainBlock);
  memory->userdata = data;
  RenderState* rs = &memory->renderState;

#if 0
  {
    ModelData data = {};
    data = ImportExternalModelData(VENOM_ASSET_FILE("axis.obj"), 0);
    FILE* file = fopen("test.txt", "wb");
    assert(file != 0);
    for(size_t i = 0; i < data.meshData.vertexCount; i++) {
      fprintf(file, "V3{%ff, %ff, %ff},\n", data.meshData.vertices[i].position.x,
        data.meshData.vertices[i].position.y, data.meshData.vertices[i].position.z);
    }
    fprintf(file, "\n");
    for(size_t i = 0; i < data.meshData.indexCount; i++) {
      fprintf(file, "%u,", data.meshData.indices[i]);
      if(i % 3 == 0) fprintf(file, "\n");
    }
  }
#endif


  InitializeCamera(&data->camera, 45*DEG2RAD, 0.1f, 100.0f, sys->screen_width, sys->screen_height);
  data->camera.position = {4, 10, 2};
  data->camera.pitch = -89.99f*DEG2RAD;
  data->camera.yaw = -90.0f*DEG2RAD;

  EntityContainerInit(&data->entityContainer, 1024, 8);

  {
    AssetManifest *assetManifest = &memory->assetManifest;
    EntityContainer *entityContainer = &data->entityContainer;
    Entity *player = CreateEntity(EntityType_Player, entityContainer);
    player->modelID = GetModelID("player", assetManifest);
#if 0
    Entity *light = CreateEntity(EntityType_StaticObject, entityContainer);
    light->position = V3(0.0f, 3.0f, 0.0f);
    light->modelID = GetModelID("lamp", assetManifest);
    light->pointLight.color = V3(1.0, 1.0, 1.0);
    light->pointLight.radius = 50.0;
#endif
  }

#if 0
  { 
    V3 boundsMin = { -2, 0, -2 };
    V3 boundsMax = { 16, 0, 16 }; 
    Vertex3D vertices[4];
    U32 indices[6];
    U32 vertexCount = 0, indexCount = 0;
    GenerateQuadFromPointsCW(
      V3{boundsMin.x, boundsMin.y, boundsMin.z},
      V3{boundsMax.x, boundsMin.y, boundsMin.z},
      V3{boundsMax.x, boundsMin.y, boundsMax.z},
      V3{boundsMin.x, boundsMin.y, boundsMax.z}, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    CalculateSurfaceNormals(vertices, vertexCount, indices, indexCount);
    CalculateVertexTangents(vertices, indices, vertexCount, indexCount);
    CreateIndexedVertexArray3D(&data->proceduralMesh, vertices, indices, 
      vertexCount, indexCount, GL_STATIC_DRAW);
  }
#endif


}
void VenomModuleUpdate(GameMemory* memory) {
  GameData* data = (GameData*)memory->userdata;
  EntityContainer* entityContainer = &data->entityContainer;
  EditorData* editorData = &data->editorData;
  RenderState* rs = &memory->renderState;
  AssetManifest *manifest = &memory->assetManifest;

  const SystemInfo* sys = &memory->systemInfo;
  InputState* input = &memory->inputState;

  Player& player = data->player;

#if 0
  const float acceleration = 15.0f;
  const float dragCoefficent = 0.9f;
  const F32 COOLDOWN_TIME = 0.2f;
  float deltaTime = memory->deltaTime;
  if(input->isKeyDown[KEYCODE_W]) 
    player.velocity -= V3(0, 0, acceleration) * deltaTime;
  if(input->isKeyDown[KEYCODE_S]) 
    player.velocity += V3{0, 0, acceleration} * deltaTime;
  if(input->isKeyDown[KEYCODE_A]) 
    player.velocity -= V3{acceleration, 0, 0} * deltaTime;
  if(input->isKeyDown[KEYCODE_D]) 
    player.velocity += V3{acceleration, 0, 0} * deltaTime;

  F32 cursorDisplacementX = input->cursorPosX - (sys->screen_width * 0.5f);
  F32 cursorDisplacementY = input->cursorPosY - (sys->screen_height * 0.5f);
  V2 displacementVector = { cursorDisplacementX, cursorDisplacementY };
  displacementVector = Normalize(displacementVector);

  const F32 BULLET_SPEED = 5000.0f;
  if(input->isButtonDown[MOUSE_LEFT] && player.cooldownTime <= 0.0f) {
    U64 id = CreateEntity(EntityType_Bullet, entityArray);
    Entity* entity = &entityArray->entities[id];
    entity->position = player.position + V3(0.0f, 0.4f, 0.0f);
    entity->velocity = { displacementVector.x * BULLET_SPEED, 
      0, displacementVector.y * BULLET_SPEED };
    entity->bullet.decayTime = 2.0f;
    entity->bullet.lightColor = V3(0.9, 0.0, 0.2);
    entity->bullet.bulletRadius = 0.05f;
    entity->bullet.lightRadius= 1.0;
    player.cooldownTime += COOLDOWN_TIME; 
  }

  player.cooldownTime -= deltaTime;
  if (player.cooldownTime < 0.0f)
    player.cooldownTime = 0.0f;

  Camera& camera = data->camera;
  player.position += player.velocity * deltaTime;
  player.velocity *= dragCoefficent;
  camera.position.x = Lerp(camera.position.x, player.position.x, 0.2f);
  camera.position.z = Lerp(camera.position.z, player.position.z, 0.2f);
#endif

  EditorData* editor = &data->editorData;


  draw_debug_plane(V3(0.0f, 0.0f, 0.0f), V3(16.0, 0.0f, 0.0f), V3(16.0f, 0.0f, 16.0f));

  if (input->isButtonDown[MOUSE_RIGHT]) {
    MoveCameraWithFPSControls(&memory->renderState.debugCamera,
      &memory->inputState, memory->deltaTime);
  } else {

#if 0
    if(input->isKeyDown[KEYCODE_X]) {
      if(editor->selectedEntities.count > 0) {
        for(size_t i = 0; i < editor->selectedEntities.count; i++){
          EntityIndex index = {};
          index.blockIndex = 0;
          index.slotIndex = editor->selectedEntities[i]; 
          DestroyEntity(index, entityContainer);
        }
        editor->selectedEntities.count = 0;
      }
    }
#endif
  }

  process_editor_mode(memory, entityContainer, editor, input, &rs->debugCamera);
}

void VenomModuleLoad(GameMemory* memory) {
  GameData* data = (GameData*)memory->userdata;
  V3 structureOrigin = { 0, 0.01F, 0 };
  V3 structureSize = { 8, 2.6F, 5 };
}



void VenomModuleRender(GameMemory* memory) {
  RenderState* rs = &memory->renderState;
  GameData* data = (GameData*)memory->userdata;
  EditorData* editorData = &data->editorData;
  EditorData* editor = &data->editorData;

  AddDirectionalLight(V3(1.0f, 0.0f, 0.0f), V3(1.0, 1.0, 1.0), &rs->drawList);
  
  const F32 deltaTime = memory->deltaTime;
  EntityContainer* entityContainer = &data->entityContainer;
  EntityBlock* block = entityContainer->firstAvaibleBlock;
  for(U64 i = 0; i < entityContainer->capacityPerBlock; i++) {
    if(block->flags[i] & EntityFlag_PRESENT) {
      Entity* entity = &block->entities[i];
      if(block->types[i] == EntityType_PointLight) {
        AddShadowCastingPointLight(entity->position, entity->pointLight.color, 
          entity->pointLight.radius, &rs->drawList);
      } 


      if (block->flags[i] & EntityFlag_Visible) {
        if(editor->selectedEntities.ContainsValue(i)){
          PushOutlinedModelDrawCommand(entity->modelID.slot_index, &rs->drawList, entity->position, entity->rotation); 
        } else {
          PushModelDrawCommand(entity->modelID.slot_index, &rs->drawList, entity->position, entity->rotation); 
        }

        ModelAsset* modelAsset = GetModelAsset(entity->modelID, &memory->assetManifest);
        assert(modelAsset != 0);

        VenomDebugRenderSettings *debugRenderSettings = GetDebugRenderSettings();
        if (debugRenderSettings->drawPhysicsColliders) {
          V3 boundsSize = Abs(modelAsset->aabb.max - modelAsset->aabb.min);
          draw_debug_box(entity->position - (boundsSize * 0.5f), entity->position + (boundsSize * 0.5f));
        }
      }
    }
  }

  VenomRenderScene(memory, &rs->debugCamera);
}
