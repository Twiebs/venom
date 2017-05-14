


static inline
void ReadWorldFile(const char *filename, EntityContainer *container, AssetManifest *manifest)
{
  if (vs::BeginFileRead(filename) == 0) return;
  while (vs::BeginGroupRead()) {
    char temp[128];
    vs::ReadString("type", temp, sizeof(temp));
    int type = FindMatchingString(temp, EntityTypeStrings, EntityType_Count);
    if (type == -1) { //TODO(Torin) Should probably do somthing smarter if this fails!
      LogError("Invalid EntityType when reading world file");
      vs::EndGroupRead();
      continue;
    }

#if 1
    Entity *e = CreateEntity((EntityType)type, container);
    vs::ReadFloat32("position", 3, &e->position);
    vs::ReadFloat32("rotation", 3, &e->rotation);
    vs::ReadString("modelName", temp, sizeof(temp));
    e->modelID = GetModelID(temp, manifest);
    if (e->modelID.slot_index == 0) {
      LogWarning("model asset %s missing", temp);
    }


    switch (type) {
    case EntityType_PointLight: {
      vs::ReadFloat32("color", 3, &e->pointLight.color);
    };
    }
#endif



    //LogDebug(temp);

    vs::EndGroupRead();
  }
  vs::EndFileRead();
}

static inline
void WriteWorldFile(const char *filename, EntityContainer *container,
  AssetManifest *manifest) {
  vs::BeginFileWrite(filename);
  EntityBlock* block = container->firstAvaibleBlock;
  for (size_t i = 0; i < container->capacityPerBlock; i++) {
    if (block->flags[i] & EntityFlag_PRESENT) {
      Entity* e = &block->entities[i];
      vs::BeginGroupWrite("entity");
      vs::WriteString("type", EntityTypeStrings[block->types[i]]);
      vs::WriteFloat32("position", 3, &e->position);
      vs::WriteFloat32("rotation", 3, &e->rotation);
      vs::WriteString("modelName", e->modelID.asset_name);
      AssetSlot *slot = &manifest->modelAssets[e->modelID.slot_index];
      switch (block->types[i]) {
      case EntityType_PointLight: {
        vs::WriteFloat32("color", 3, &e->pointLight.color);
      } break;
      }
      vs::EndGroupWrite();
    }
  }
  vs::EndFileWrite();
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
      ModelAsset* modelAsset = (ModelAsset *)assetManifest->modelAssets[entity->modelID.slot_index].asset;
     
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

static inline void update_editor_selection(U32 index, EditorData *editor, EntityContainer *ec, AssetManifest *am) {
  assert(index != INVALID_ENTITY_INDEX);
  switch (editor->selectMode) {
  case EditorSelectMode_Default: {
    editor->selectedEntities.count = 0;
    editor->selectedEntities.PushBack(index);
  } break;
  case EditorSelectMode_Append: {
    if (!editor->selectedEntities.ContainsValue(index)) {
      editor->selectedEntities.PushBack(index);
    }
  } break;
  case EditorSelectMode_Remove: {
    editor->selectedEntities.RemoveFirstValueUnordered(index);
  } break;
  }

  editor->groupAABB = { V3(FLT_MAX), V3(FLT_MIN) };
  for (size_t i = 0; i < editor->selectedEntities.count; i++) {
    Entity *e = GetEntity(editor->selectedEntities[i], ec);
    ModelAsset *asset = GetModelAsset(e->modelID, am);
    AABB entityAABB = { e->position - (asset->size * 0.5f),
      e->position + (asset->size * 0.5f) };
    for (size_t i = 0; i < 3; i++) {
      editor->groupAABB.min[i] = Min(entityAABB.min[i], editor->groupAABB.min[i]);
      editor->groupAABB.max[i] = Max(entityAABB.max[i], editor->groupAABB.max[i]);
    }
  }

  editor->originalGroupPosition = Center(editor->groupAABB);
  editor->currentGroupPosition = editor->originalGroupPosition;
  editor->originalEntityPositions.count = 0;
  fori(editor->selectedEntities.count) {
    Entity *e = GetEntity(editor->selectedEntities[i], ec);
    editor->originalEntityPositions.PushBack(e->position);
  }
}

static inline void SetVisualizerActive(EditorData *editor, B8 state) {
  editor->isVisualizerActive = state;
}

void process_editor_input(EditorData *editor, InputState *input, F32 deltaTime) {


  if (editor->isVisualizerActive) {
    if (input->isButtonDown[MOUSE_RIGHT]) {
      MoveCameraWithFPSControls(&editor->visualizerCamera, input, deltaTime);
    } else {
      editor->isVisualizerActive = false;
    }

    
    return;
  }

  if (input->isKeyDown[KEYCODE_X]) {
    editor->transformConstraints = EditorTransformConstraint_XAxis;
  } else if (input->isKeyDown[KEYCODE_Y]) {
    editor->transformConstraints = EditorTransformConstraint_YAxis;
  } else if (input->isKeyDown[KEYCODE_Z]) {
    editor->transformConstraints = EditorTransformConstraint_ZAxis;
  }

  if (input->isKeyDown[KEYCODE_SHIFT]) {
    editor->selectMode = EditorSelectMode_Append;
  } else if (input->isKeyDown[KEYCODE_LCONTROL]) {
    editor->selectMode = EditorSelectMode_Remove;
  } else {
    editor->selectMode = EditorSelectMode_Default;
  }

  if (input->isButtonDown[MOUSE_LEFT] && !ImGui::IsMouseHoveringAnyWindow()) {
    editor->activeCommand = EditorCommand_Select;
  }

  if (input->isKeyDown[KEYCODE_E]) {
    editor->activeCommand = EditorCommand_MeshEdit;
  }

  if (input->isKeyDown[KEYCODE_G]) {
    if (input->isKeyDown[KEYCODE_SHIFT]) {
      editor->activeCommand = EditorCommand_Grab;
    } else {
      editor->activeCommand = EditorCommand_Project;
    }
  }

  if (input->isKeyDown[KEYCODE_CAPSLOCK]) {
    editor->activeCommand = EditorCommand_None;
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





static inline void DrawEditorSearchWindow(EditorData *editor) {
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;
  ImGui::Begin("Search", 0, ImVec2(600, 800), 1.0f, flags);

  struct FuzzyFindEntry {
    int score;
    AssetSlot *slot;
    U32 index;
  };

  static char inputBuffer[128] = {};
  static DynamicArray<FuzzyFindEntry> fuzzyScores;

  InputState *input = &GetVenomEngineData()->inputState;

  ImGui::SetKeyboardFocusHere();
  if (ImGui::InputText("Search", inputBuffer, sizeof(inputBuffer)), ImGuiInputTextFlags_EnterReturnsTrue) {
    if (input->isKeyDown[KEYCODE_ENTER]) {
      if (fuzzyScores.count > 0) {
        FuzzyFindEntry *entry = &fuzzyScores[0];
        memset(inputBuffer, 0x00, sizeof(inputBuffer));
        fuzzyScores.count = 0;
        editor->isSearchWindowOpen = false;
        editor->viewMode = EditorViewMode_Assets;
        editor->selectedAssetType = AssetType_MODEL;
        editor->selectedIndex = entry->index;
        editor->isEditorVisible = true;
      }
    }

  }

  fuzzyScores.count = 0;
  AssetManifest *assets = GetAssetManifest();
  for (size_t i = 0; i < assets->modelAssets.count; i++) {
    AssetSlot *slot = &assets->modelAssets[i];
    int fuzzyScore = CalculateFuzzyScore(inputBuffer, slot->name);
    if (fuzzyScore != -1) {
      FuzzyFindEntry *entry = fuzzyScores.AddElement();
      entry->score = fuzzyScore;
      entry->slot = slot;
      entry->index = i;
    }
  }

  ImGui::Columns(2);
  for (size_t i = 0; i < fuzzyScores.count; i++) {
    FuzzyFindEntry *entry = &fuzzyScores[i];
    ImGui::Text(entry->slot->name);
    ImGui::NextColumn();
    ImGui::Text("Model");
    ImGui::NextColumn();
  }

  ImGui::End();
}

#if 0
static void DrawSkeletonTransformInfo(Animation_State *animState, ModelAsset *model) {
  if (model->data.jointCount > 0) {
    static S32 selected_joint = -1;
    static S32 selectedAnimationClip = -1;
    static F32 currentAnimationTime = 0.0f;

    ImGui::BeginChildFrame(0, ImVec2(400, 200));
    imgui_draw_joint_tree(0, model->data.joints, &selected_joint);
    S32 next_root_node_index = model->data.joints->sibling_index;
    while (next_root_node_index != -1) {
      Animation_Joint *next = &model->data.joints[next_root_node_index];
      imgui_draw_joint_tree(next_root_node_index, model->data.joints, &selected_joint);
      next_root_node_index = next->sibling_index;
    }

      ImGui::EndChildFrame();
      ImGui::SameLine();
      ImGui::BeginChildFrame(1, ImVec2(400, 400));


      if (selected_joint != -1) {

        //TODO(Torin) Temporary Memory to hold each joint info
        Animation_State animationState = {};
        animationState.model_id = GetModelID(slot->name, manifest);
        animationState.animationStateCount = 1;
        AnimationClipState *clipState = &animationState.animationStates[0];
        clipState->animationClipID = selectedAnimationClip;
        clipState->animationLocalTime = currentAnimationTime;

        Animation_Joint *joint = &asset->data.joints[selected_joint];

        //TODO(Torin) Dynamic temporary memory!
        M4 localPoses[64];
        M4 globalPoses[64];
        CalculateLocalPosesForSkeleton(asset->data.joints, asset->data.jointCount, &animationState, localPoses);
        CalculateGlobalPosesForSkeleton(asset->data.joints, asset->data.jointCount, localPoses, globalPoses);
        M4 skinningMatrix = CalculateSkinningMatrix(joint, globalPoses[selected_joint]);

        ImGui::Text("Joint Name: %s", joint->name);

        M4 inverseLocalTransform = Inverse(joint->localTransform);
        M4 localOffsetMatrix = inverseLocalTransform * localPoses[selected_joint];
        Transform localTransform = DecomposeTransformationMatrix(localPoses[selected_joint]);
        Transform localOffsetTransform = DecomposeTransformationMatrix(localOffsetMatrix);
        Transform globalTransform = DecomposeTransformationMatrix(globalPoses[selected_joint]);
        Transform skinningTransform = DecomposeTransformationMatrix(skinningMatrix);
        ImGuiTransform("LocalPose Transform", localTransform);
        ImGuiMatrix("LocalPose Matrix", localPoses[selected_joint]);
        ImGuiTransform("LocalOffset Transform", localOffsetTransform);
        ImGuiMatrix("LocalOffset Matrix", localOffsetMatrix);
        ImGuiTransform("GlobalPose Transform", globalTransform);
        ImGuiTransform("Skinning Transform", skinningTransform);
      }

      ImGui::EndChildFrame();


      ImGui::BeginChildFrame(2, ImVec2(400, 200));
      for (size_t i = 0; i < asset->data.animation_clip_count; i++) {
        Animation_Clip *clip = &asset->data.animation_clips[i];
        if (ImGui::Selectable(clip->name)) {
          selectedAnimationClip = i;
        }
      }
      ImGui::EndChildFrame();

      if (selectedAnimationClip != -1) {
        Animation_Clip *clip = &asset->data.animation_clips[selectedAnimationClip];
        ImGui::SliderFloat("Time", &currentAnimationTime, 0.0f, clip->durationInTicks);
      }

    }
  }


} break;
}
#endif


void ProcessEditorCommand(EditorData* editor, Camera* camera, GameMemory* memory, EntityContainer* entityContainer) {
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
      V4 ray = ProjectViewportCoordsToWorldSpaceRay(camera, input->cursorPosX, 
        input->cursorPosY, sys->screenWidth, sys->screenHeight);
        
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
          EntityIndex hack = {};
          hack.slotIndex = editor->selectedEntities[i];
          Entity *e = GetEntity(hack, ec);
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
      V4 worldRay = ProjectViewportCoordsToWorldSpaceRay(camera, input->cursorPosX, input->cursorPosY + 40,
        sys->screenWidth, sys->screenHeight);
      
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
            ModelAsset* modelAsset = (ModelAsset *)assetManifest->modelAssets[entity->modelID.slot_index].asset;
            if (modelAsset == nullptr) continue;
           
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
        update_editor_selection(hitEntityIndex, editor, entityContainer, &memory->assetManifest);
        draw_debug_sphere(point, 0.1f, COLOR_YELLOW, true, 5.0f); 
        draw_debug_line(camera->position, (camera->position + (worldRay * tmin)), COLOR_YELLOW, true, 5);
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
        block->flags[editor->selectedEntities[0]] &= ~EntityFlag_VISIBLE;
      }

      Entity *e = GetEntity(editor->selectedEntities[0], ec);
      ModelAsset *asset = GetModelAsset(e->modelID, am);
      for(size_t i = 0; i < asset->vertexCount; i++){
        draw_debug_sphere(asset->vertices[i].position + e->position, 0.01f, COLOR_YELLOW, true);
      }

    } break;
  }

  if(editor->selectedEntities.count > 0){
    draw_debug_axes(Center(editor->groupAABB));
    //AddSphere(drawList, Center(editor->groupAABB), 1.0);
  }

  editor->lastCommand = editor->activeCommand;
}

static inline void RenderGJKStepVisualization(PrimitiveRenderer *renderer, CollisionVisualization *collison, size_t index) {
  GJKStep *step = &collison->gjkSteps[index];

  for (size_t i = 0; i < step->simplexCount; i++) {
    renderer->Sphere(step->simplex[i], 0.2f, COLOR_BLUE, true);
  }

  switch (step->simplexCount) {
    case 2: {
      const V3& a = step->simplex[1];
      const V3& b = step->simplex[0];
      renderer->Text("A", a);
      renderer->Text("B", b);
      renderer->Line(a, b, COLOR_YELLOW);
    } break;
    case 3: {
      const V3& a = step->simplex[2];
      const V3& b = step->simplex[1];
      const V3& c = step->simplex[0];
      renderer->Text("A", a);
      renderer->Text("B", b);
      renderer->Text("C", c);
      renderer->Line(a, c, COLOR_YELLOW);
      renderer->Line(a, b, COLOR_YELLOW);
      renderer->Line(c, b, COLOR_YELLOW);
    } break;

    case 4: {
      const V3& a = step->simplex[3];
      const V3& b = step->simplex[2];
      const V3& c = step->simplex[1];
      const V3& d = step->simplex[0];
      renderer->Text("A", a);
      renderer->Text("B", b);
      renderer->Text("C", c);
      renderer->Text("D", d);

      renderer->Line(b, c, COLOR_YELLOW);
      renderer->Line(b, d, COLOR_YELLOW);
      renderer->Line(c, d, COLOR_YELLOW);

      renderer->Line(a, b, COLOR_YELLOW);
      renderer->Line(a, c, COLOR_YELLOW);
      renderer->Line(a, d, COLOR_YELLOW);
      
#if 0
      renderer->Line(step->simplex[0], step->simplex[1], COLOR_YELLOW);
      renderer->Line(step->simplex[1], step->simplex[2], COLOR_YELLOW);
      renderer->Line(step->simplex[2], step->simplex[0], COLOR_YELLOW);

      renderer->Line(step->simplex[0], step->simplex[3], COLOR_YELLOW);
      renderer->Line(step->simplex[1], step->simplex[3], COLOR_YELLOW);
      renderer->Line(step->simplex[2], step->simplex[3], COLOR_YELLOW);
#endif
    } break;

  }
}

static inline V3 TriangleCentroid(V3 a, V3 b, V3 c) {
  V3 result;
  result.x = (a.x + b.x + c.x) / 3.0f;
  result.y = (a.y + b.y + c.y) / 3.0f;
  result.z = (a.z + b.z + c.z) / 3.0f;
  return result;
}

static inline void RenderEPAStepVisualization(PrimitiveRenderer *renderer, CollisionVisualization *collison, size_t index) {
  EPAStep *step = &collison->epaSteps[index];
  if (collison->showPenetrationVector) {
    renderer->Line(V3(0.0f), collison->penetrationVector, COLOR_RED);
  }

  renderer->DirectionalLight(V3(1.0f, -0.5f, 0.0f));
  renderer->isLightingEnabled = collison->renderSolidTriangles;
  for (size_t i = 0; i < step->triangleCount; i++) {
    auto& triangle = step->triangles[i];
    renderer->Triangle(triangle.a, triangle.b, triangle.c, COLOR_YELLOW, collison->renderSolidTriangles);
    V3 centroid = TriangleCentroid(triangle.a, triangle.b, triangle.c);
    renderer->Line(centroid, centroid + triangle.normal, COLOR_GREEN);
  }
  renderer->isLightingEnabled = false;

  if (collison->showClosestTriangle) {
    auto& triangle = step->closestTriangle;
    renderer->Line(triangle.a, triangle.b, COLOR_MAGENTA);
    renderer->Line(triangle.b, triangle.c, COLOR_MAGENTA);
    renderer->Line(triangle.c, triangle.a, COLOR_MAGENTA);
  }
  
}

void RenderVisualizerFramebuffer(EditorData *editor, F32 deltaTime) {
  Camera *camera = &editor->visualizerCamera;
  UpdateCamera(&editor->visualizerCamera);
  //NOTE(Torin) ImGui wants textures with the origin in the top left corner
  camera->viewMatrix = Scale(1.0f, -1.0f, 1.0f) * camera->viewMatrix;
 
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_STENCIL_TEST);

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, editor->visualizerFramebuffer);
  glViewport(0, 0, editor->visualizerWidth, editor->visualizerHeight);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  PhysicsSimulation *sim = &GetEngine()->physicsSimulation;
  CollisionVisualization *collison = &sim->collisionVisualization;
  PrimitiveRenderer *renderer = &editor->visualizerRenderer;

  renderer->Sphere(V3(0.0f), 0.2f, COLOR_MAGENTA, true);
  renderer->Axis(V3(0.0f));
  if (collison->hideMinkowskiRegion == false) {
    AABB minkowskiRegion = MinkowskiDifference(collison->a, collison->b);
    renderer->Box(minkowskiRegion);
  }

  if (editor->gjkStepIndex >= collison->gjkSteps.count) {
    if (editor->gjkStepIndex < (collison->gjkSteps.count + collison->epaSteps.count)) {
      size_t index = editor->gjkStepIndex - collison->gjkSteps.count;
      RenderEPAStepVisualization(renderer, collison, index);
    } else {
      editor->gjkStepIndex = 0;
    }
  } else {
    RenderGJKStepVisualization(renderer, collison, editor->gjkStepIndex);
  }

  renderer->Render(&editor->visualizerCamera, deltaTime);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool process_editor_mode(GameMemory *memory, EntityContainer *entityContainer, EditorData *editor, InputState *input, Camera * camera) {
  process_editor_input(editor, input, memory->deltaTime);



  auto engine = GetEngine();

  if (engine->isConsoleVisible) {
    bool scrollToBottom = engine->unseenErrorCount > 0 || engine->unseenInfoCount > 0 || engine->unseenWarningCount > 0;
    DrawConsole(&engine->debugLog, scrollToBottom);
  } else if (engine->unseenErrorCount > 0 || engine->unseenInfoCount > 0 || engine->unseenWarningCount > 0) {
    static const U32 OVERLAY_WIDTH = 250;
    static const U32 OVERLAY_PADDING = 8;
    const ImGuiIO& io = ImGui::GetIO();
    const U32 xpos = io.DisplaySize.x - OVERLAY_WIDTH - OVERLAY_PADDING;
    ImGui::SetNextWindowPos(ImVec2(xpos, OVERLAY_PADDING + 40));
    ImGui::SetNextWindowSize(ImVec2(OVERLAY_WIDTH - OVERLAY_PADDING, 0));
    ImGui::Begin("EventOverlay", 0, ImVec2(0, 0), 0.3f,
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    if (engine->unseenErrorCount > 0) {
      ImGui::TextColored(ImColor(255, 0, 0), "Errors: %u", engine->unseenErrorCount);
      ImGui::SameLine();
    }
    if (engine->unseenWarningCount > 0) {
      ImGui::TextColored(ImColor(255, 255, 0), "Warnings: %u", engine->unseenWarningCount);
    }
    ImGui::End();
  }



  if (editor->isSearchWindowOpen) {
    DrawEditorSearchWindow(editor);
  }

  if (editor->isVisualizerVisible) {
    DrawVisualizationWindow(editor);
  }


  if (editor->isEditorVisible) {
    ImGui::Begin("Editor");
    if (ImGui::Button("Entities")) editor->viewMode = EditorViewMode_Entities;
    ImGui::SameLine();
    if (ImGui::Button("Assets")) editor->viewMode = EditorViewMode_Assets;
    ImGui::SameLine();
    if (ImGui::Button("Physics")) editor->viewMode = EditorViewMode_Physics;
    ImGui::SameLine();
    if (ImGui::Button("Render")) editor->viewMode = EditorViewMode_Render;
    ImGui::SameLine();
    if (ImGui::Button("Game")) editor->viewMode = EditorViewMode_Game;

    AssetManifest *assets = GetAssetManifest();
    switch (editor->viewMode) {

    case EditorViewMode_Entities: {
      DrawEditorEntityTab(editor, assets, entityContainer);
    } break;

    case EditorViewMode_Assets: {
      AssetManifest *assets = GetAssetManifest();
      DrawEditorAssetTab(editor, assets);
    } break;

    case EditorViewMode_Physics: {
      ShowEditorPhysicsTab(editor);
    } break;

    case EditorViewMode_Render: {
      ShowEditorRenderTab(&memory->renderState.debugRenderFrameInfo, &memory->renderState.debugRenderSettings);
    } break;

    case EditorViewMode_Game: {
      DrawEditorGameTab(camera);
    } break;


    }

    ImGui::End();
  }

  ProcessEditorCommand(editor, camera, memory, entityContainer);



  if (ImGui::IsMouseHoveringAnyWindow() && ImGui::IsMouseClicked(1)) {
    return true;
  } else if (editor->isVisualizerActive) {
    return true;
  }

  return false;
}



void InitalizeEditor(EditorData *editor) {
  editor->selectedAssetType = AssetType_MODEL;
  editor->lastSelectedIndex = -1;
  editor->selectedIndex = -1;

  editor->visualizerHeight = 512;
  editor->visualizerWidth = 512;

  glGenFramebuffers(1, &editor->visualizerFramebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, editor->visualizerFramebuffer);
  
  glGenTextures(1, &editor->visualizerColorBuffer);
  glBindTexture(GL_TEXTURE_2D, editor->visualizerColorBuffer);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, editor->visualizerWidth, editor->visualizerHeight);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glGenTextures(1, &editor->visualizerDepthBuffer);
  glBindTexture(GL_TEXTURE_2D, editor->visualizerDepthBuffer);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, editor->visualizerWidth, editor->visualizerHeight);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, editor->visualizerColorBuffer, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, editor->visualizerDepthBuffer, 0);
  GLuint attachments[] = {
    GL_COLOR_ATTACHMENT0,
  };

  glDrawBuffers(1, attachments);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  auto sysInfo = &GetVenomEngineData()->systemInfo;
  InitializeCamera(&editor->visualizerCamera, 45.0f*DEG2RAD, 0.001f, 100.0f, editor->visualizerWidth, editor->visualizerHeight);
  editor->visualizerCamera.position = V3(0.0f, 0.0f, 4.0f);
  InitializeCamera(&editor->editorCamera, 45.0f * DEG2RAD, 0.01f, 1000.0f, sysInfo->screenWidth, sysInfo->screenHeight);
  editor->editorCamera.position = V3(0.0f, 4.0f, 0.0f);

  InitFontVertexBuffer(&g_fontBuffer, 256);

  ImGuiSetStyle();
}