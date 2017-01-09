
inline void ImGuiV3(const char *name, V3& v) {
  ImGui::InputFloat3(name, &v.x, 3, ImGuiInputTextFlags_ReadOnly);
}

inline void ImGuiTransform(const char *name, Transform& t) {
  V3 eulerRotation = QuaternionToEuler(t.rotation);
  eulerRotation *= RAD2DEG;
  ImGui::Text(name);
  ImGuiV3("translation", t.translation);
  ImGuiV3("rotation", eulerRotation);
  ImGuiV3("scale", t.scale);
}

inline void ImGuiMatrix(const char *name, M4& m) {
  ImGui::Text("%.2f, %.2f, %.2f, %.2f", m[0][0], m[1][0], m[2][0], m[3][0]);
  ImGui::Text("%.2f, %.2f, %.2f, %.2f", m[0][1], m[1][1], m[2][1], m[3][1]);
  ImGui::Text("%.2f, %.2f, %.2f, %.2f", m[0][2], m[1][2], m[2][2], m[3][2]);
  ImGui::Text("%.2f, %.2f, %.2f, %.2f", m[0][3], m[1][3], m[2][3], m[3][3]);
}

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
V4 ProjectViewportCoordsToWorldSpaceRay(int viewportX, int viewportY, int viewportWidth, int viewportHeight, Camera *camera) {
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

void process_editor_input(EditorData *editor, InputState *input) {
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

void draw_entity_editor_ui(EditorData *editor, AssetManifest *manifest, EntityContainer *entityContainer) {
  ImGui::Text("Active Editor Command: %s", EditorCommandNames[editor->activeCommand]);
  ImGui::Text("SelectedEntityCount: %d", (int)editor->selectedEntities.count);
  ImGui::SameLine();
  if (ImGui::Button("Clear Selection")) {
    editor->selectedEntities.count = 0;
  }

  if (ImGui::Button("Save World")) {
    char temp[512];
    sprintf(temp, "world%d.vsf", (int)time(0));
    WriteWorldFile(temp, entityContainer, manifest);
    WriteWorldFile("../project/world.vsf", entityContainer, manifest);
  }

  ImGui::SameLine();
  if (ImGui::Button("Load World")) {
    ReadWorldFile("../project/world.vsf", entityContainer, manifest);
  }

  ImGui::SameLine();
  if (ImGui::Button("CreateEntity")) {
    CreateEntity(EntityType_StaticObject, entityContainer);
  }

  if (ImGui::Button("DestroyEntity")) {
    for (size_t i = 0; i < editor->selectedEntities.count; i++) {
      EntityIndex index = {};
      index.blockIndex = 0;//TODO(Torin)
      index.slotIndex = editor->selectedEntities[i];
      DestroyEntity(index, entityContainer);
    }
  }

  ImGui::Columns(2);
  ImGui::Separator();
  ImGui::BeginChild("EntityList");

  //TODO(Torin)
  EntityBlock* block = entityContainer->firstAvaibleBlock;
  for (size_t i = 0; i < entityContainer->capacityPerBlock; i++) {
    if (block->flags[i] & EntityFlag_PRESENT) {
      const Entity& entity = block->entities[i];
      ImGui::PushID(i);

      size_t listIndex = 0;
      bool containsIndex = editor->selectedEntities.ContainsValue(i, &listIndex);
      if (ImGui::Selectable(EntityTypeStrings[block->types[i]], containsIndex)) {
        update_editor_selection(i, editor, entityContainer, manifest);
      }
      ImGui::PopID();
    }
  }


  ImGui::EndChild();
  ImGui::NextColumn();
  ImGui::BeginChild("Info");
  if (editor->selectedEntities.count > 0) {
    //Entity& entity = block->entities[editor->selectedEntityIndex];
    ImGui::DragFloat3("Position", &editor->currentGroupPosition.x, 0.1f);
    ImGui::DragFloat3("Rotation", &editor->currentGroupRotation.x, 0.1f);

    if (editor->selectedEntities.count == 1) {
      Entity *entity = GetEntity(editor->selectedEntities[0], entityContainer);
      AssetSlot *slot = &manifest->modelAssets[entity->modelID.slot_index];
      ModelAsset *modelAsset = GetModelAsset(entity->modelID, manifest);
      if (ImGui::Button(slot->name)) {
        ImGui::OpenPopup("ModelSelect");
      }

      if (ImGui::BeginPopup("ModelSelect")) {
        for (size_t i = 0; i < manifest->modelAssets.count; i++) {
          if (ImGui::Selectable(manifest->modelAssets[i].name)) {
            entity->modelID = GetModelID(manifest->modelAssets[i].name, manifest);
          }
        }
        ImGui::EndPopup();
      }
    }

  }
  ImGui::EndChild();
  ImGui::NextColumn();
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



static inline void imgui_draw_joint_tree(S32 joint_index, Animation_Joint *joint_list, S32 *selected_index) {
  Animation_Joint *joint = &joint_list[joint_index];
  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick;
  bool is_node_open = ImGui::TreeNodeEx(joint->name, flags);
  if (ImGui::IsItemClicked()) {
    *selected_index = joint_index;
  }

  if (is_node_open) {
    S32 child_index = joint->child_index;
    while (child_index != -1) {
      Animation_Joint *child = &joint_list[child_index];
      imgui_draw_joint_tree(child_index, joint_list, selected_index);
      child_index = child->sibling_index;
    }
    ImGui::TreePop();
  }
}

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


static void draw_asset_manifest_ui(EditorData *editor, AssetManifest* manifest) {

  if (ImGui::Combo("Asset Type", &editor->selectedAssetType, AssetTypeNames, AssetType_COUNT)) {
    editor->selectedIndex = -1;
    editor->lastSelectedIndex = -1;
  }
  ImGui::SameLine();
  if (ImGui::Button("SaveManifest")) {
    char temp[512];
    sprintf(temp, "assets%d.vsf", (int)time(0));
    WriteAssetManifestFile(temp, manifest);
    VenomCopyFile(temp, "../project/assets.vsf");
  }

  ImGui::Separator();
  ImGui::Columns(2);


  DynamicArray<AssetSlot> *slotArray = &manifest->assetSlotArrays[editor->selectedAssetType];
  ImGui::BeginChild("AssetList");
  ImGui::Columns(2);
  ImGui::Text("Name");
  ImGui::NextColumn();
  ImGui::Text("Status");
  ImGui::NextColumn();
  ImGui::Separator();

  for (size_t i = 0; i < slotArray->count; i++) {
    AssetSlot *slot = &slotArray->data[i];
    ImGui::PushID(i);
    if (ImGui::Selectable(slot->name, editor->selectedIndex == (int)i)) {
      editor->selectedIndex = i;
    }
    ImGui::PopID();

    //NOTE(Torin) We don't care about the locks here if we read a bogus value
    //it doesnt matter the developer will only see it for 16ms
    ImGui::NextColumn();
    static const ImColor loadedColor = ImColor(0.0f, 1.0f, 0.0f, 1.0f);
    static const ImColor unloadedColor = ImColor(1.0f, 0.0f, 0.0f, 1.0f);
    ImColor color = (slot->assetState == AssetState_Loaded) ? loadedColor : unloadedColor;

    const char *text = "INVALID";
    if (slot->assetState & AssetState_Invalid) text = "invalid";
    else if (slot->assetState & AssetState_Loaded) text = "loaded";
    else text = "unloaded";

    ImGui::TextColored(color, text);
    ImGui::NextColumn();
  }

  ImGui::EndChild();

  ImGui::BeginChild("Buttons");
  ImGui::Text("Selcted Index: %d", editor->selectedIndex);
  ImGui::SameLine();
  if (ImGui::Button("New Asset")) {
    ImGui::OpenPopup("New Asset");
  }
  ImGui::SameLine();
  if (ImGui::Button("Delete Asset")) {
    if (editor->selectedIndex != -1) {
      RemoveModelFromManifest(editor->selectedIndex, manifest);
      editor->selectedIndex = -1;
      editor->lastSelectedIndex = -1;
      manifest->modelReloadCounter++;
    }
  }
  ImGui::EndChild();

  

  

  if (ImGui::BeginPopup("New Asset")) {
    if (ShowModelCreateWidgets(manifest)) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  ImGui::NextColumn();
  if (editor->selectedIndex != -1) {


    if (editor->selectedIndex != editor->lastSelectedIndex) {
      if (editor->lastSelectedIndex != -1 && editor->lastSelectedIndex != 0) {
        
      }



    }

    static char nameBuffer[256] = {};
    static char filenameBuffer[256] = {};
    AssetSlot *slot = &slotArray->data[editor->selectedIndex];
    bool name_modifed = ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
    bool filename_modifed = ImGui::InputText("Filename", filenameBuffer, sizeof(filenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
    if (name_modifed || filename_modifed || (editor->selectedIndex != editor->lastSelectedIndex)) {
      if (editor->lastSelectedIndex != -1 && editor->lastSelectedIndex != 0) {
        AssetSlot *lastSlot = &manifest->modelAssets[editor->lastSelectedIndex];
        SpinLock(&lastSlot->lock);
        MemoryFree(lastSlot->name);
        MemoryFree(lastSlot->filename);
        lastSlot->name = strdup(nameBuffer);
        lastSlot->filename = strdup(filenameBuffer);
        ReleaseLock(&lastSlot->lock);
      }

      editor->lastSelectedIndex = editor->selectedIndex;
      SpinLock(&slot->lock);
      strcpy(nameBuffer, slot->name);
      strcpy(filenameBuffer, slot->filename);
      ReleaseLock(&slot->lock);
    }
    
    if (slot->assetState == AssetState_Loaded) {
      switch (editor->selectedAssetType) {
      case AssetType_MODEL: {
        ModelAsset *asset = (ModelAsset *)slot->asset;
        for (size_t i = 0; i < asset->meshCount; i++) {
          ShowMaterialDataInfo(&asset->materialDataPerMesh[i], editor->lastSelectedIndex != editor->selectedIndex);
        }

        if (asset->jointCount > 0) {
          if (ImGui::CollapsingHeader("Animation")) {
            static S32 selected_joint = -1;
            static S32 selectedAnimationClip = -1;
            static F32 currentAnimationTime = 0.0f;

            ImGui::BeginChildFrame(0, ImVec2(400, 200));
            imgui_draw_joint_tree(0, asset->joints, &selected_joint);
            S32 next_root_node_index = asset->joints->sibling_index;
            while (next_root_node_index != -1) {
              Animation_Joint *next = &asset->joints[next_root_node_index];
              imgui_draw_joint_tree(next_root_node_index, asset->joints, &selected_joint);
              next_root_node_index = next->sibling_index;
            }
            
            ImGui::EndChildFrame();
            ImGui::SameLine();
            ImGui::BeginChildFrame(1, ImVec2(400, 400));


            if (selected_joint != -1) {

              //TODO(Torin) Temporary Memory to hold each joint info
              AnimationState animationState = {};
              animationState.model_id = GetModelID(slot->name, manifest);
              animationState.animationStateCount = 1;
              AnimationClipState *clipState = &animationState.animationStates[0];
              clipState->animationClipID = selectedAnimationClip;
              clipState->localTimeSeconds = currentAnimationTime;

              Animation_Joint *joint = &asset->joints[selected_joint];

              //TODO(Torin) Dynamic temporary memory!
              M4 localPoses[64];
              M4 globalPoses[64];
              CalculateLocalPosesForSkeleton(asset->joints, asset->jointCount, &animationState, localPoses);
              CalculateGlobalPosesForSkeleton(asset->joints, asset->jointCount, localPoses, globalPoses);
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
            for (size_t i = 0; i < asset->animationClipCount; i++) {
              Animation_Clip *clip = &asset->animationClips[i];
              if (ImGui::Selectable(clip->name)) {
                selectedAnimationClip = i;
              }
            }
            ImGui::EndChildFrame();

            if (selectedAnimationClip != -1) {
              Animation_Clip *clip = &asset->animationClips[selectedAnimationClip];
              ImGui::SliderFloat("Time", &currentAnimationTime, 0.0f, clip->durationInTicks);
            }

          }
        }


      } break;

      case AssetType_MATERIAL: {
        const MaterialAsset *material = &manifest->materialAssetList.materials[editor->selectedIndex];
        static const U32 textureDisplaySize = 256;
        ImVec2 textureBounds = ImVec2(textureDisplaySize, textureDisplaySize);
        ImGui::BeginChild("diffuse", ImVec2(textureDisplaySize, textureDisplaySize + 18));
        ImGui::Text("Diffuse Texture");
        ImGui::Image((ImTextureID)(uintptr_t)material->data.diffuseTextureID, textureBounds);
        ImGui::EndChild();

        if (material->data.materialFlags & MaterialFlag_NORMAL) {
          ImGui::SameLine();
          ImGui::BeginChild("normal", ImVec2(textureDisplaySize, textureDisplaySize + 18));
          ImGui::Text("Normal Texture");
          ImGui::Image((ImTextureID)(uintptr_t)material->data.normalTextureID, textureBounds);
          ImGui::EndChild();
        }

        if (material->data.materialFlags & MaterialFlag_SPECULAR) {
          ImGui::SameLine();
          ImGui::BeginChild("specular", ImVec2(textureDisplaySize, textureDisplaySize + 18));
          ImGui::Text("Specular Texture");
          ImGui::Image((ImTextureID)(uintptr_t)material->data.specularTextureID, textureBounds);
          ImGui::EndChild();
        }
      } break;
      }
    }
  }

  ImGui::NextColumn();
}

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
      V4 worldRay = ProjectViewportCoordsToWorldSpaceRay(input->cursorPosX, input->cursorPosY + 40,
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



void process_editor_mode(GameMemory *memory, EntityContainer *entityContainer, EditorData *editor, InputState *input, Camera * camera) {
  process_editor_input(editor, input);

  auto engine = GetEngine();

  if (engine->isConsoleVisible) {
    bool scrollToBottom = engine->unseenErrorCount > 0 || engine->unseenInfoCount > 0 || engine->unseenWarningCount > 0;
    ShowConsole(&engine->debugLog, scrollToBottom);
  } else if (engine->unseenErrorCount > 0 || engine->unseenInfoCount > 0 || engine->unseenWarningCount > 0){
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

  if (editor->isEditorVisible) {
    ImGui::Begin("Editor");
    if (ImGui::Button("Entities")) editor->viewMode = EditorViewMode_Entities;
    ImGui::SameLine();
    if (ImGui::Button("Assets")) editor->viewMode = EditorViewMode_Assets;
    ImGui::SameLine();
    if (ImGui::Button("Debug")) editor->viewMode = EditorViewMode_Debug;
    ImGui::SameLine();
    if (ImGui::Button("Player")) editor->viewMode = EditorViewMode_Player;


    AssetManifest *assets = GetAssetManifest();
    switch (editor->viewMode) {
    case EditorViewMode_Entities: {
      draw_entity_editor_ui(editor, assets, entityContainer);
    } break;

    case EditorViewMode_Assets: {
      AssetManifest *assets = GetAssetManifest();
      draw_asset_manifest_ui(editor, assets);
    } break;

    case EditorViewMode_Debug: {
      draw_debug_render_info_ui(&memory->renderState.debugRenderFrameInfo, &memory->renderState.debugRenderSettings);
      ImGui::BeginGroup();
      //draw_profiler_ui(&engine->profileData, &memory->mainBlock);
      ImGui::EndGroup();
    } break;

    case EditorViewMode_Player: {
      draw_camera_info_ui(camera);
    } break;

  }

  ImGui::End();
  }

  ProcessEditorCommand(editor, camera, memory, entityContainer);

}


void InitalizeEditor(EditorData *editor) {
  editor->selectedAssetType = -1;
  editor->lastSelectedIndex = -1;
  editor->selectedIndex = -1;
}