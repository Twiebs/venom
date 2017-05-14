#define VENOM_ENTITY_METALIST_MACRO EntityTypeList
#define VENOM_ENTITY_STRUCT Entity
#define VENOM_MATERIAL_LIST_FILE "assets/asset_list.h"

#include "venom_module.cpp"

#include "venom_entity.cpp"

struct GameData {
  Camera camera;
  EntityContainer entityContainer;
  bool initalized;
  Player player;
  IndexedVertexArray procedural ;
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
  InitalizeTerrainGenerator(&data->terrain, V3(0.0, 0.0, 0.0), &memory->mainBlock);
  EndProfileEntry();

  rs->terrain = &data->terrain;
  GetEngine()->physicsSimulation.terrain = &data->terrain;

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

 
  InitializeCamera(&data->camera, 45*DEG2RAD, 0.1f, 10000.0f, sys->screenWidth, sys->screenHeight);
  data->camera.position = {4, 10, 2};


  EntityContainerInit(&data->entityContainer, 1024, 8);

  {
    AssetManifest *assetManifest = &memory->assetManifest;
    EntityContainer *entityContainer = &data->entityContainer;
    EntityIndex player_index;
    Entity *player = CreateEntity(EntityType_Player, &player_index, entityContainer);
    assign_model_to_entity(player_index, GetModelID("player", assetManifest), assetManifest, entityContainer);
    ModelAsset *asset = GetModelAsset(player->modelID, assetManifest);
    player->animation_state.model_id = player->modelID;
    player->position = player->position += V3(1, 5.0f, 1);
    data->playerEntityIndex = player_index;

#if 0
    Orientation cameraOrientation = CalculateCameraOrientationForTrackTarget(player->position);
    data->camera.position = cameraOrientation.position;
    V3 eulerRotation = QuaternionToEuler(cameraOrientation.rotation);
    data->camera.pitch = eulerRotation.x;
    data->camera.yaw = eulerRotation.y;
#endif

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

  Entity *player = GetEntity(data->playerEntityIndex, entityContainer);

  EditorData* editor = &memory->editor;

  bool editorHandledInput = process_editor_mode(memory, entityContainer, editor, input, &editor->editorCamera);




  Engine *engine = GetEngine();
  FinalizeEngineTasks(engine);


  const F32 deltaTime = memory->deltaTime;

  if (engine->isPaused == false) {
    if (editorHandledInput == false) {
      PlayerInput playerInput = KeyboardAndMouseToPlayerInput(input);
      if (playerInput.lookModeActive == false) {
        UpdateMousePicker(&data->camera);
      }

      ThirdPersonPlayerControl(player, &data->camera, &playerInput, memory->deltaTime);
    }

    //UpdateTerrainFromViewPosition(&data->terrain, player->position);
    UpdateAnimationStates(entityContainer, deltaTime);
    SimulatePhysics(&engine->physicsSimulation, entityContainer);
  }

  
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

  const V3 sunDirection = Normalize(V3(1.0, 0.5, 0.0f));
  AddDirectionalLight(sunDirection, V3(1.0, 1.0, 1.0), &rs->drawList);

  auto sunModelID = GetModelID("Sun", GetAssetManifest());
  auto sunModel = GetModelAsset(sunModelID);
  AddStaticModelToDrawList(&rs->drawList, sunModel, sunDirection * 10);

  
  
  const F32 deltaTime = memory->deltaTime;
  EntityContainer* entityContainer = &data->entityContainer;
  
  auto engine = GetEngine();



  EntityBlock* block = entityContainer->firstAvaibleBlock;
  for (U64 i = 0; i < entityContainer->capacityPerBlock; i++) {
    if (block->flags[i] & EntityFlag_PRESENT) {
      Entity* entity = &block->entities[i];

      if(block->types[i] == EntityType_PointLight) {
        AddShadowCastingPointLight(entity->position, entity->pointLight.color, 
          entity->pointLight.radius, &rs->drawList);
      }

      ModelAsset *model = GetModelAsset(entity->modelID, &memory->assetManifest);
      if (model == nullptr) continue;
      bool is_entity_animated = model->jointCount > 0;

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
      }
    }
  }

  Camera *camera = nullptr;
  camera = &data->camera;
#if 0
  if (editor->isEditorVisible) {
    //camera = &editor->editorCamera;
    ..draw_debug_camera(&data->camera);
  } else {
    
  }
#endif


  //ImGui::ShowTestWindow();
  
  rs->newTerrain = &engine->terrain;

  VenomRenderScene(memory, camera);
  RenderVisualizerFramebuffer(editor, memory->deltaTime);
}
