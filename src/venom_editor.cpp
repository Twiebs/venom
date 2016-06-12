
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

static inline
void UpdateEditorSelection(U32 index, EditorData *editor,
EntityContainer *ec, AssetManifest *am) 
{
  assert(index != INVALID_ENTITY_INDEX);
  switch(editor->selectMode){
    case EditorSelectMode_Default: {
      editor->selectedEntities.count = 0;
      editor->selectedEntities.PushBack(index);
    } break;
    case EditorSelectMode_Append: {
      if(!editor->selectedEntities.ContainsValue(index)) {
        editor->selectedEntities.PushBack(index);
      }
    } break;
    case EditorSelectMode_Remove: {
      editor->selectedEntities.RemoveFirstValueUnordered(index);
    } break;
  }

  editor->groupAABB = { V3(FLT_MAX), V3(FLT_MIN) };
  fori(editor->selectedEntities.count){
    Entity *e = GetEntity(editor->selectedEntities[i], ec);
    const ModelAsset *asset = GetModelAsset(e->modelID, am);
    AABB entityAABB = { e->position - (asset->size * 0.5f),
      e->position + (asset->size * 0.5f) }; 
    for(size_t i = 0; i < 3; i++){
      editor->groupAABB.min[i] = Min(entityAABB.min[i], editor->groupAABB.min[i]);
      editor->groupAABB.max[i] = Max(entityAABB.max[i], editor->groupAABB.max[i]);
    }
  }

  editor->originalGroupPosition = Center(editor->groupAABB);
  editor->currentGroupPosition = editor->originalGroupPosition;
  editor->originalEntityPositions.count = 0;
  fori(editor->selectedEntities.count){
    Entity *e = GetEntity(editor->selectedEntities[i], ec);
    editor->originalEntityPositions.PushBack(e->position);
  }
}

#if 0
static inline
void UpdateEntityGroupAABB(EditorData *editor, EntityContainer *ec, AssetManifest *am){
  editor->groupAABB = { V3(FLT_MAX), V3(FLT_MIN) };
  fori(editor->selectedEntities.count){
    Entity *e = GetEntity(editor->selectedEntities[i], ec);
    const ModelAsset *asset = GetModelAsset(e->modelID, am);
    AABB entityAABB = { e->position - (asset->size * 0.5f),
      e->position + (asset->size * 0.5f) }; 
    for(size_t i = 0; i < 3; i++){
      editor->groupAABB.min[i] = Min(entityAABB.min[i], editor->groupAABB.min[i]);
      editor->groupAABB.max[i] = Max(entityAABB.max[i], editor->groupAABB.max[i]);
    }
  }

  editor->originalGroupPosition = Center(editor->groupAABB);
  editor->currentGroupPosition = editor->originalGroupPosition;
  editor->originalEntityPositions.count = 0;
  fori(editor->selectedEntities.count){
    Entity *e = GetEntity(editor->selectedEntities[i], ec);
    editor->originalEntityPositions.PushBack(e->position);
  }
}
#endif

void ProcessEditorCommand(EditorData* editor, Camera* camera, GameMemory* memory,
    EntityContainer* entityContainer)
{
  static const U32 INVALID_ENTITY_INDEX = ((((U64)1) << 32) - 1);

  InputState* input = &memory->inputState;
  SystemInfo* sys = &memory->systemInfo;
  VenomDrawList *drawList = &memory->renderState.drawList;
  
  AssetManifest *am = &memory->assetManifest;
  EntityContainer *ec = entityContainer;


  switch(editor->activeCommand) {
    case EditorCommand_None: {
      editor->transformConstraints = (EditorTransformConstraint)0;
    } break;

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

#if 0
      if(editor->lastCommand != EditorCommand_Project){
        editor->groupAABB = { V3(FLT_MAX), V3(FLT_MIN) };
        fori(editor->selectedEntities.count){
          Entity *e = GetEntity(editor->selectedEntities[i], ec);
          const ModelAsset *asset = GetModelAsset(e->modelID, am);
          AABB entityAABB = { e->position - (asset->size * 0.5f),
            e->position + (asset->size * 0.5f) }; 
          for(size_t i = 0; i < 3; i++){
            editor->groupAABB.min[i] = Min(entityAABB.min[i], editor->groupAABB.min[i]);
            editor->groupAABB.max[i] = Max(entityAABB.max[i], editor->groupAABB.max[i]);
          }
        }

        editor->originalGroupPosition = Center(editor->groupAABB);
        editor->currentGroupPosition = editor->originalGroupPosition;
        editor->originalEntityPositions.count = 0;
        fori(editor->selectedEntities.count){
          Entity *e = GetEntity(editor->selectedEntities[i], ec);
          editor->originalEntityPositions.PushBack(e->position);
        }
      }
      
#endif
      V4 ray = ProjectViewportCoordsToWorldSpaceRay(input->cursorPosX, 
        input->cursorPosY, sys->screen_width, sys->screen_height, camera);
        
      V3 hitPoint = {};
      U32 hitEntityIndex = RayCastAgainstEntitiesAABB(ray, 
        entityContainer, &memory->assetManifest, camera, &hitPoint, 
        editor->selectedEntities.data, editor->selectedEntities.count);

      if(hitEntityIndex != INVALID_ENTITY_INDEX) {
        editor->currentGroupPosition = hitPoint;
        V3 totalDisplacement = editor->currentGroupPosition - 
          editor->originalGroupPosition; 
        
        if(editor->transformConstraints & EditorTransformConstraint_XAxis){
          totalDisplacement = V3{totalDisplacement.x, 0.0f, 0.0f};
        } else if (editor->transformConstraints & EditorTransformConstraint_YAxis){
          totalDisplacement = V3{0.0f, totalDisplacement.y, 0.0f};
        } else if (editor->transformConstraints & EditorTransformConstraint_ZAxis){
          totalDisplacement = V3{0.0f, 0.0f, totalDisplacement.z};
        }

        fori(editor->selectedEntities.count){
          Entity *e = GetEntity(editor->selectedEntities[i], ec);
          e->position = editor->originalEntityPositions[i] + totalDisplacement +
            V3{ 0, Abs(editor->groupAABB.max - editor->groupAABB.min).y * 0.5f, 0};
        }


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

      //TODO(Torin) Inline UpdateEditorSelection? 
      if(hitEntityIndex != INVALID_ENTITY_INDEX) {
        UpdateEditorSelection(hitEntityIndex, editor, entityContainer, 
          &memory->assetManifest);
        AddSphere(drawList, point, 0.1f, COLOR_YELLOW, true, 5.0f); 
        AddLine(drawList, camera->position, 
          (camera->position + (worldRay * tmin)), COLOR_YELLOW, true, 5);
      }
       
      editor->activeCommand = EditorCommand_None;
    } break;



    case EditorCommand_MeshEdit: {
      if(editor->selectedEntities.count == 0 ||
         editor->selectedEntities.count > 1) {
        return;
      }

      if(editor->lastCommand != EditorCommand_MeshEdit){
        EntityBlock *block = ec->firstAvaibleBlock;
        block->flags[editor->selectedEntities[0]] &= ~EntityFlag_Visible;
      }

      Entity *e = GetEntity(editor->selectedEntities[0], ec);
      ModelAsset *asset = GetModelAsset(e->modelID, am);
      MeshData *data = &asset->data.meshData;
      for(size_t i = 0; i < data->vertexCount; i++){
        AddSphere(drawList, data->vertices[i].position + e->position, 0.01f,
           COLOR_YELLOW, true);
      }

    } break;

  }

  if(editor->selectedEntities.count > 0){
    AddAxes(Center(editor->groupAABB), drawList);
    //AddSphere(drawList, Center(editor->groupAABB), 1.0);
  }

  editor->lastCommand = editor->activeCommand;
}
