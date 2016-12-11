
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

static inline void imgui_matrix(M4 m) {
  ImGui::Text("%.2f, %.2f, %.2f, %.2f", m[0][0], m[1][0], m[2][0], m[3][0]);
  ImGui::Text("%.2f, %.2f, %.2f, %.2f", m[0][1], m[1][1], m[2][1], m[3][1]);
  ImGui::Text("%.2f, %.2f, %.2f, %.2f", m[0][2], m[1][2], m[2][2], m[3][2]);
  ImGui::Text("%.2f, %.2f, %.2f, %.2f", m[0][3], m[1][3], m[2][3], m[3][3]);
}

static void draw_asset_manifest_ui(AssetManifest* manifest) {
  static int selectedAssetType = 0;
  static int lastSelectedIndex = -1;
  static int selectedIndex = -1;

  if (ImGui::Combo("Asset Type", &selectedAssetType, AssetTypeNames, AssetType_COUNT)) {
    selectedIndex = -1;
    lastSelectedIndex = -1;
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


  DynamicArray<AssetSlot> *slotArray = &manifest->assetSlotArrays[selectedAssetType];
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
    if (ImGui::Selectable(slot->name, selectedIndex == (int)i)) {
      selectedIndex = i;
    }
    ImGui::PopID();

    ImGui::NextColumn();
    static const ImColor loadedColor = ImColor(0.0f, 1.0f, 0.0f, 1.0f);
    static const ImColor unloadedColor = ImColor(1.0f, 0.0f, 0.0f, 1.0f);
    ImColor color = (slot->flags & AssetFlag_LOADED) ? loadedColor : unloadedColor;

    const char *text = "INVALID";
    if (slot->flags & AssetFlag_INVALID) text = "invalid";
    else if (slot->flags & AssetFlag_LOADED) text = "loaded";
    else text = "unloaded";

    ImGui::TextColored(color, text);
    ImGui::NextColumn();
  }

  ImGui::EndChild();

  ImGui::BeginChild("Buttons");
  ImGui::Text("Selcted Index: %d", selectedIndex);
  ImGui::SameLine();
  if (ImGui::Button("New Asset")) {
    ImGui::OpenPopup("New Asset");
  }
  ImGui::SameLine();
  if (ImGui::Button("Delete Asset")) {
    if (selectedIndex != -1) {
      RemoveModelFromManifest(selectedIndex, manifest);
      selectedIndex = -1;
      lastSelectedIndex = -1;
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
  if (selectedIndex != -1) {


    if (selectedIndex != lastSelectedIndex) {
      if (lastSelectedIndex != -1 && lastSelectedIndex != 0) {
        
      }



    }

    static char nameBuffer[256] = {};
    static char filenameBuffer[256] = {};
    AssetSlot *slot = &slotArray->data[selectedIndex];
    bool name_modifed = ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
    bool filename_modifed = ImGui::InputText("Filename", filenameBuffer, sizeof(filenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
    if (name_modifed || filename_modifed || (selectedIndex != lastSelectedIndex)) {
      if (lastSelectedIndex != -1 && lastSelectedIndex != 0) {
        AssetSlot *lastSlot = &manifest->modelAssets[lastSelectedIndex];
        free(lastSlot->name);
        free(lastSlot->filename);
        lastSlot->name = strdup(nameBuffer);
        lastSlot->filename = strdup(filenameBuffer);
      }
      lastSelectedIndex = selectedIndex;
      strcpy(nameBuffer, slot->name);
      strcpy(filenameBuffer, slot->filename);
    }
    




    if (slot->asset != 0) {
      switch (selectedAssetType) {
      case AssetType_MODEL: {
        ModelAsset *asset = (ModelAsset *)slot->asset;
        for (size_t i = 0; i < asset->data.meshCount; i++) {
          ShowMaterialDataInfo(&asset->data.materialDataPerMesh[i], lastSelectedIndex != selectedIndex);
        }

        ImGui::BeginChildFrame(0, ImVec2(400, 200));
        static int selected_bone = -1;
        for (size_t i = 0; i < asset->data.meshData.jointCount; i++) {
          Animation_Joint *joint = &asset->data.meshData.joints[i];
          if (ImGui::Selectable(joint->name, selected_bone == i)) {
            if (selected_bone == i) {
              selected_bone = -1;
            } else {
              selected_bone = i;
            }
          }
        }


        if (selected_bone != -1) {
          ImGui::Text("Inverse Bind Pose");
          imgui_matrix(asset->data.meshData.joints[selected_bone].inverse_bind_matrix);
          ImGui::Text("Parent Realtive");
          imgui_matrix(asset->data.meshData.joints[selected_bone].parent_realtive_matrix);
        }

        ImGui::EndChildFrame();



      } break;

      case AssetType_MATERIAL: {
        const MaterialAsset *material = &manifest->materialAssetList.materials[selectedIndex];
        static const U32 textureDisplaySize = 256;
        ImVec2 textureBounds = ImVec2(textureDisplaySize, textureDisplaySize);
        ImGui::BeginChild("diffuse", ImVec2(textureDisplaySize, textureDisplaySize + 18));
        ImGui::Text("Diffuse Texture");
        ImGui::Image((ImTextureID)(uintptr_t)material->drawable.diffuse_texture_id, textureBounds);
        ImGui::EndChild();

        if (material->data.materialFlags & MaterialFlag_NORMAL) {
          ImGui::SameLine();
          ImGui::BeginChild("normal", ImVec2(textureDisplaySize, textureDisplaySize + 18));
          ImGui::Text("Normal Texture");
          ImGui::Image((ImTextureID)(uintptr_t)material->drawable.normal_texture_id, textureBounds);
          ImGui::EndChild();
        }

        if (material->data.materialFlags & MaterialFlag_SPECULAR) {
          ImGui::SameLine();
          ImGui::BeginChild("specular", ImVec2(textureDisplaySize, textureDisplaySize + 18));
          ImGui::Text("Specular Texture");
          ImGui::Image((ImTextureID)(uintptr_t)material->drawable.specular_texture_id, textureBounds);
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
            ModelAsset* modelAsset = 
              (ModelAsset *)assetManifest->modelAssets[entity->modelID.slot_index].asset;
           
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
      MeshData *data = &asset->data.meshData;
      for(size_t i = 0; i < data->vertexCount; i++){
        draw_debug_sphere(data->vertices[i].position + e->position, 0.01f, COLOR_YELLOW, true);
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


  VenomDebugData* debugData = &memory->debugData;

  if (debugData->isConsoleVisible) {
    bool scrollToBottom = debugData->unseenErrorCount > 0 || debugData->unseenInfoCount > 0 || debugData->unseenWarningCount > 0;
    ShowConsole(debugData, &memory->debugData.debugLog, scrollToBottom);
  } else if (debugData->unseenErrorCount > 0 || debugData->unseenInfoCount > 0 || debugData->unseenWarningCount > 0){
    static const U32 OVERLAY_WIDTH = 250;
    static const U32 OVERLAY_PADDING = 8;
    const ImGuiIO& io = ImGui::GetIO();
    const U32 xpos = io.DisplaySize.x - OVERLAY_WIDTH - OVERLAY_PADDING;
    ImGui::SetNextWindowPos(ImVec2(xpos, OVERLAY_PADDING + 40));
    ImGui::SetNextWindowSize(ImVec2(OVERLAY_WIDTH - OVERLAY_PADDING, 0));
    ImGui::Begin("EventOverlay", 0, ImVec2(0, 0), 0.3f,
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    if (debugData->unseenErrorCount > 0) {
      ImGui::TextColored(ImColor(255, 0, 0), "Errors: %u", debugData->unseenErrorCount);
      ImGui::SameLine();
    }
    if (debugData->unseenWarningCount > 0) {
      ImGui::TextColored(ImColor(255, 255, 0), "Warnings: %u", debugData->unseenWarningCount);
    }
    ImGui::End();
  }

  if (memory->debugData.isEditorVisible) {
    ImGui::Begin("Editor");
    static int current_editor_view = 0;
    if (ImGui::Button("Entities")) current_editor_view = 0;
    ImGui::SameLine();
    if (ImGui::Button("Assets")) current_editor_view = 1;
    ImGui::SameLine();
    if (ImGui::Button("Debug")) current_editor_view = 2;
    ImGui::SameLine();
    if (ImGui::Button("Player")) current_editor_view = 3;

    if (current_editor_view == 0) {
      draw_entity_editor_ui(editor, &memory->assetManifest, entityContainer);
    } else if (current_editor_view == 1) {
      draw_asset_manifest_ui(&memory->assetManifest);
    } else if (current_editor_view == 2) {
      draw_debug_render_info_ui(&memory->renderState.debugRenderFrameInfo, &memory->renderState.debugRenderSettings);
      ImGui::BeginGroup();
      draw_profiler_ui(&memory->debugData.profileData, &memory->mainBlock);
      ImGui::EndGroup();
    } else if (current_editor_view == 3) {
      draw_camera_info_ui(camera);
    }

    ImGui::End();
    ProcessEditorCommand(editor, camera, memory, entityContainer);
  }
}
