#define VENOM_ENTITY_METALIST_MACRO EntityTypeList
#define VENOM_ENTITY_STRUCT Entity
#define VENOM_MATERIAL_LIST_FILE "assets/asset_list.h"

#include "venom_module.cpp"
#include "venom_editor.cpp"
#include "venom_entity.cpp"

#include "terrain.cpp"

#include "Game/CameraMovement.cpp"
#include "Game/CharacterMovement.cpp"

struct GameData {
  Camera camera;
  EntityContainer entityContainer;
  bool initalized;
  Player player;
  IndexedVertexArray proceduralMesh;
  EntityIndex playerEntityIndex;
  TerrainGenerationState terrain;
};

void ScatterInRectangle(RNGSeed *seed, F32 x, F32 y, F32 w, F32 h, U32 rows, U32 cols, std::function<void(V2)> procedure) {
  F32 xstep = w / cols;
  F32 ystep = h / rows;
  for (size_t iy = 0; iy < cols + 1; iy++) {
    for (size_t ix = 0; ix < rows + 1; ix++) {
      F32 p = 0.4f;
      V2 point;
      point.x = x + (ix * xstep) + (RandomInRange(-p, p, *seed) * xstep);
      point.y = y + (iy * ystep) + (RandomInRange(-p, p, *seed) * ystep);
      procedure(point);
    }
  }
}


void VenomModuleStart(GameMemory* memory) {
  SystemInfo* sys = &memory->systemInfo;
  GameData* data = PushStruct(GameData, &memory->mainBlock);
  memory->userdata = data;
  RenderState* rs = &memory->renderState;
  
  BeginProfileEntry("Initalize Terrain Generator");
  InitalizeTerrainGenerator(&data->terrain, &memory->mainBlock, V3(0.0, 0.0, 0.0));
  EndProfileEntry();

  rs->terrain = &data->terrain;

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


  InitializeCamera(&data->camera, 45*DEG2RAD, 0.1f, 10000.0f, sys->screen_width, sys->screen_height);
  data->camera.position = {4, 10, 2};
  data->camera.pitch = -89.99f*DEG2RAD;
  data->camera.yaw = -90.0f*DEG2RAD;


  EntityContainerInit(&data->entityContainer, 1024, 8);

  {
    AssetManifest *assetManifest = &memory->assetManifest;
    EntityContainer *entityContainer = &data->entityContainer;
    EntityIndex player_index;
    Entity *player = CreateEntity(EntityType_Player, &player_index, entityContainer);
    assign_model_to_entity(player_index, GetModelID("player", assetManifest), assetManifest, entityContainer);
    ModelAsset *asset = GetModelAsset(player->modelID, assetManifest);
    player->animation_state.model_id = player->modelID;
    data->playerEntityIndex = player_index;

    Orientation cameraOrientation = CalculateCameraOrientationForTrackTarget(player->position);
    data->camera.position = cameraOrientation.position;
    V3 eulerRotation = QuaternionToEuler(cameraOrientation.rotation);
    data->camera.pitch = eulerRotation.x;
    data->camera.yaw = eulerRotation.y;

    RNGSeed seed(15);
    ScatterInRectangle(&seed, -128, -128, 256, 256, 8, 8, [&](V2 point) {
      Entity *e = CreateEntity(EntityType_StaticObject, entityContainer);
      e->modelID = GetModelID("Tree", assetManifest);
      e->position.x = point.x;
      e->position.z = point.y;
    });

  }
}



void VenomModuleUpdate(GameMemory* memory) {
  GameData* data = (GameData*)memory->userdata;
  EntityContainer* entityContainer = &data->entityContainer;
  EditorData* editorData = &memory->editor;
  RenderState* rs = &memory->renderState;
  AssetManifest *manifest = &memory->assetManifest;

  const SystemInfo* sys = &memory->systemInfo;
  InputState* input = &memory->inputState;

  //Player& player = data->player;

#if 0
  const float acceleration = 15.0f;
  const float dragCoefficent = 0.9f;
  const F32 COOLDOWN_TIME = 0.2f;
  float deltaTime = memory->deltaTime;


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

  EditorData* editor = &memory->editor;

  Entity *player = GetEntity(data->playerEntityIndex, entityContainer);
  UpdateTerrainFromViewPosition(&data->terrain, player->position);
  TrackPositionWithCamera(player->position, &data->camera);

  if (input->isButtonDown[MOUSE_RIGHT] && editor->isEditorVisible) {
    MoveCameraWithFPSControls(&memory->renderState.debugCamera,
      &memory->inputState, memory->deltaTime);
  } else {
    static CharacterMovementParameters params;
    params.acceleration = 30.0f;
    params.maxVelocity = 8.0f;
    params.stopScalar = 0.9f;
    MovementInput movementInput = KeyboardToMovementInput(input);
    MoveEntityWithThirdPersonCharacterMovement(player, &movementInput, &params, memory->deltaTime);
  }


#if 0
if (input->isKeyDown[KEYCODE_X]) {
  if (editor->selectedEntities.count > 0) {
    for (size_t i = 0; i < editor->selectedEntities.count; i++) {
      EntityIndex index = {};
      index.blockIndex = 0;
      index.slotIndex = editor->selectedEntities[i];
      DestroyEntity(index, entityContainer);
}
    editor->selectedEntities.count = 0;
      }
    }
#endif



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
  EditorData* editorData = &memory->editor;
  EditorData* editor = editorData;

  AddDirectionalLight(V3(1.0f, 0.5f, 0.0f), V3(1.0, 1.0, 1.0), &rs->drawList);
  
  const F32 deltaTime = memory->deltaTime;
  EntityContainer* entityContainer = &data->entityContainer;
  EntityBlock* block = entityContainer->firstAvaibleBlock;
  for (U64 i = 0; i < entityContainer->capacityPerBlock; i++) {
    if (block->flags[i] & EntityFlag_PRESENT) {
      Entity* entity = &block->entities[i];
      UpdateEntityPhysics(entity, deltaTime);

      if(block->types[i] == EntityType_PointLight) {
        AddShadowCastingPointLight(entity->position, entity->pointLight.color, 
          entity->pointLight.radius, &rs->drawList);
      }

      ModelAsset *model = GetModelAsset(entity->modelID, &memory->assetManifest);
      if (model == nullptr) continue;
      bool is_entity_animated = model->jointCount > 0;
      if (is_entity_animated) {
        if (entity->animation_state.isInitalized == false) {
          InitalizeAnimationState(&entity->animation_state, model);
        }

        UpdateAnimationState(&entity->animation_state, model, memory->deltaTime);
      }

      if (block->flags[i] & EntityFlag_VISIBLE) {        
        if (editor->selectedEntities.ContainsValue(i)) {
          V3 rotation = QuaternionToEuler(entity->rotation);
          AddOutlinedModelToDrawList(&rs->drawList, model, entity->position, rotation); 
        } else if (is_entity_animated) {
          V3 rotation = QuaternionToEuler(entity->rotation);
          V3 position = entity->position;
          position.y += model->size.y * 0.5f;
          AddAnimatedModelToDrawList(&rs->drawList, model, &entity->animation_state, position, rotation);
        } else {
          V3 rotation = QuaternionToEuler(entity->rotation);
          V3 position = entity->position;
          position.y += model->size.y * 0.5f;
          AddStaticModelToDrawList(&rs->drawList, model, position, rotation); 
        }
        
        VenomDebugRenderSettings *debugRenderSettings = GetDebugRenderSettings();
        if (debugRenderSettings->drawPhysicsColliders) {
          V3 boundsMin = entity->position - (model->size * 0.5f);
          V3 boundsMax = entity->position + (model->size* 0.5f);
          boundsMin.y += model->size.y * 0.5f;
          boundsMax.y += model->size.y * 0.5f;
          draw_debug_box(boundsMin, boundsMax, COLOR_GREEN);
        }
      }
    }
  }

  Camera *camera = nullptr;
  if (editor->isEditorVisible) {
    camera = &rs->debugCamera;
    draw_debug_camera(&data->camera);
  } else {
    camera = &data->camera;
  }

  
  VenomRenderScene(memory, camera);
}
