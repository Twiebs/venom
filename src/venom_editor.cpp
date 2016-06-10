
static inline
V4 ProjectViewportCoordsToWorldSpaceRay(int viewportX, int viewportY, int viewportWidth, int viewportHeight, Camera *camera)
{
  float x = (((float)viewportX / (float)viewportWidth) * 2.0f) - 1.0f;
  float y = (((float)viewportY / (float)viewportHeight) * 2.0f) - 1.0f;
  V4 screenspaceRay = { x, y, -1.0f, 1.0f };
  V4 viewspaceRay = Inverse(camera->projection) * screenspaceRay;
  viewspaceRay = V4{ viewspaceRay.x, -viewspaceRay.y, -1.0f, 0.0 };
  V4 worldRay = Normalize(Inverse(camera->view) * viewspaceRay);
  return worldRay;
}

static inline
U32 RayCastAgainstEntitiesAABB(V4 ray, EntityContainer *entityContainer, 
AssetManifest *assetManifest, Camera* camera, V3 
*outPoint, U32 *ignoredEntityList, U32 ignoredEntityCount) 
{
  float lastTmin = 100000.0f;
  float tmin = 0.0f;
  V3 point = {};

  U32 result = INVALID_ENTITY_INDEX;
  EntityBlock* block = entityContainer->firstAvaibleBlock;
  for(U64 i = 0; i < entityContainer->capacityPerBlock; i++) {
    if(block->flags[i] & EntityFlag_PRESENT) {
      Entity* entity = &block->entities[i];
      ModelAsset* modelAsset = (ModelAsset *)
        assetManifest->modelAssets[entity->modelID].asset;
     
      V3 boundsSize = Abs(modelAsset->aabb.max - modelAsset->aabb.min); 
      AABB collider = { entity->position - (boundsSize * 0.5),
        entity->position + (boundsSize * 0.5) };

      if(IntersectRayAABB(camera->position, V3(ray), collider, tmin, point)){
        for(size_t ignoredIndex = 0; i < ignoredEntityCount; i++) {
          if(i == ignoredEntityList[ignoredIndex]) continue;
        }

        if(tmin < lastTmin) {
          lastTmin = tmin;
          result = i;
        }
      }
    }
  }

  *outPoint = point;
  return result;
}

void ProcessEditorCommand(EditorData* editor, Camera* camera, GameMemory* memory,
    EntityContainer* entityContainer)
{
  static const U32 INVALID_ENTITY_INDEX = ((((U64)1) << 32) - 1);

  InputState* input = &memory->inputState;
  SystemInfo* sys = &memory->systemInfo;

  switch(editor->activeCommand) {
    case EditorCommand_Grab: {
      if(editor->selectedEntities.count == 0) {
        editor->activeCommand = EditorCommand_None;
        return;
      }

      //Entity* e = GetEntity(editor->selectedEntityIndex, entityContainer);
      //e->position = camera->position + (camera->front * 5.0f);
    } break;

    case EditorCommand_Project: 
    {
      if(editor->selectedEntities.count == 0) {
        editor->activeCommand = EditorCommand_None;
        return;
      }

      V4 ray = ProjectViewportCoordsToWorldSpaceRay(input->cursorPosX, 
        input->cursorPosY, sys->screen_width, sys->screen_height, camera);
        
      V3 hitPoint = {};
      U32 hitEntityIndex = RayCastAgainstEntitiesAABB(ray, 
        entityContainer, &memory->assetManifest, camera, &hitPoint, 
        editor->selectedEntities.data, editor->selectedEntities.count);

      if(hitEntityIndex != INVALID_ENTITY_INDEX) {
#if 0
        AssetManifest *am = &memory->assetManifest;
        Entity *selectedEntity = GetEntity(editor->selectedEntityIndex, entityContainer);
        Entity *hitEntity = GetEntity(hitEntityIndex, entityContainer);
        ModelAsset* modelAsset = (ModelAsset *)
          am->modelAssets[selectedEntity->modelID].asset;
        V3 size = (modelAsset->aabb.max - modelAsset->aabb.min);
        selectedEntity->position = hitPoint + V3{0, size.y * 0.5f, 0};
#endif
      }
    } break;

    case EditorCommand_Select: {
      V4 worldRay = ProjectViewportCoordsToWorldSpaceRay(input->cursorPosX, input->cursorPosY,
        sys->screen_width, sys->screen_height, camera);
      
      //Iterate over the entities and see if any of the rays intersect!
      AssetManifest* assetManifest = &memory->assetManifest;
      //EntityContainer* entityContainer = &data->entityContainer;
      EntityBlock* block = entityContainer->firstAvaibleBlock;

      float lastTmin = 100000.0f;
      float tmin = 0.0f;
      V3 point = {};

      U32 hitEntityIndex = INVALID_ENTITY_INDEX;
      for(U64 i = 0; i < entityContainer->capacityPerBlock; i++) {
        if(block->flags[i] & EntityFlag_PRESENT) {
          if(block->types[i] == EntityType_StaticObject){
            Entity* entity = &block->entities[i];
            ModelAsset* modelAsset = 
              (ModelAsset *)assetManifest->modelAssets[entity->modelID].asset;
           
            V3 boundsSize = Abs(modelAsset->aabb.max - modelAsset->aabb.min); 
            AABB collider = { entity->position - (boundsSize * 0.5),
              entity->position + (boundsSize * 0.5) };

            if(IntersectRayAABB(camera->position, V3(worldRay), collider, tmin, point)){
              if(tmin < lastTmin) {
                hitEntityIndex = i;
                lastTmin = tmin;
              }
            }
          }
        }
      } 

      editor->selectedEntities.PushBack(hitEntityIndex);

#if 0 
      if(didIntersect) { 
        AddSphere(&rs->drawList, point, 0.1f, COLOR_WHITE, true, 0.5); 
        AddLine(&rs->drawList, camera->position, 
            (camera->position + (worldRay * 10)), COLOR_YELLOW, true, 5);
      }
#endif
      editor->activeCommand = EditorCommand_None;
    } break;

  }
}
