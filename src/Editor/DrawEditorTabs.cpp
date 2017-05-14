
static inline void DrawEditorEntityTab(EditorData *editor, AssetManifest *manifest, EntityContainer *entityContainer) {
  ImGui::Text("Active Editor Command: %s", EditorCommandNames[editor->activeCommand]);
  ImGui::Text("SelectedEntityCount: %d", (int)editor->selectedEntities.count);
  ImGui::SameLine();
  if (ImGui::Button("Clear Selection")) {
    editor->selectedEntities.count = 0;
  }

  if (ImGui::Button("Save World")) {
    char temp[512];
    sprintf(temp, "world%d.vsf", (int)time(0));
    //WriteWorldFile(temp, entityContainer, manifest);
    //WriteWorldFile("../project/world.vsf", entityContainer, manifest);
  }

  ImGui::SameLine();
  if (ImGui::Button("Load World")) {
    //ReadWorldFile("../project/world.vsf", entityContainer, manifest);
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

static void DrawEditorGameTab(const Camera* camera) {
  auto engine = GetEngine();
  ImGui::Text("Camera Position: [%f, %f, %f]", camera->position.x, camera->position.y, camera->position.z);
  ImGui::Checkbox("Pause Game", &engine->isPaused);
}


static void DrawEditorAssetTab(EditorData *editor, AssetManifest* manifest) {

  ImGui::PushItemWidth(100.0f);
  if (ImGui::Combo("Asset Type", &editor->selectedAssetType, AssetTypeNames, AssetType_COUNT)) {
    editor->selectedIndex = -1;
    editor->lastSelectedIndex = -1;
  }
  ImGui::PopItemWidth();

  ImGui::SameLine();
  if (ImGui::Button("SaveManifest")) {
    char temp[512];
    sprintf(temp, "assets%d.vsf", (int)time(0));
    WriteAssetManifestFile(temp, manifest);
    VenomCopyFile(temp, "../project/assets.vsf");
  }

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
        AquireLock(&lastSlot->lock);
        MemoryFree(lastSlot->name);
        MemoryFree(lastSlot->filename);
        lastSlot->name = CStringDuplicate(nameBuffer);
        lastSlot->filename = CStringDuplicate(filenameBuffer);
        ReleaseLock(&lastSlot->lock);
      }

      editor->lastSelectedIndex = editor->selectedIndex;
      AquireLock(&slot->lock);
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
              CalculateLocalPosesForSkeleton(asset, &animationState, localPoses);
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

static inline void ShowEditorRenderTab(VenomDebugRenderFrameInfo* debugInfo, VenomDebugRenderSettings* renderSettings) {
  ImGui::BeginGroup();
  ImGui::Checkbox("isWireframeEnabled", &renderSettings->isWireframeEnabled);
  ImGui::Checkbox("isDebugCameraActive", &renderSettings->isDebugCameraActive);
  ImGui::Checkbox("disableCascadedShadowMaps", &renderSettings->disableCascadedShadowMaps);
  ImGui::Checkbox("disableAtmosphere", &renderSettings->disableAtmosphere);

  ImGui::Checkbox("renderDebugNormals", &renderSettings->renderDebugNormals);
  ImGui::Checkbox("renderFromDirectionalLight",
    &renderSettings->renderFromDirectionalLight);
  ImGui::Checkbox("drawDepthMap", &renderSettings->drawDepthMap);
  ImGui::EndGroup();

  ImGui::SameLine();
  ImGui::BeginGroup();
  IMGUI_U64(debugInfo, totalVerticesDrawn);
  IMGUI_U64(debugInfo, totalDrawCalls);
  IMGUI_U64(debugInfo, totalDrawListCommandsExecuted);
  IMGUI_U64(debugInfo, totalDrawListsRendered);
  ImGui::Text("directionaLightCount: %lu", debugInfo->directionalLightCount);
  ImGui::Text("pointLightCount: %lu", debugInfo->pointLightCount);
  ImGui::Text("shadowCastingPointLightCount: %lu",
    debugInfo->shadowCastingPointLightCount);
  ImGui::EndGroup();
}

static inline void ShowEditorPhysicsTab(EditorData *editor) {
  PhysicsSimulation *sim = &GetEngine()->physicsSimulation;
  CollisionVisualization *collision = &sim->collisionVisualization;

  auto settings = GetDebugRenderSettings();
  ImGui::BeginGroup();
  ImGui::Checkbox("Draw Broadphase Volumes", &settings->drawBroadphaseVolumes);
  ImGui::EndGroup();

  ImGui::BeginGroup();
  ImGui::Text("GJK Steps");
  ImGui::BeginChildFrame(0, ImVec2(400, 250));
  for (size_t i = 0; i < collision->gjkSteps.count; i++) {
    char buffer[1024];
    sprintf(buffer, "%u", (U32)i);
    if (ImGui::Selectable(buffer, editor->gjkStepIndex == i)) {
      editor->gjkStepIndex = i;
    }
  }
  ImGui::EndChildFrame();
  ImGui::EndGroup();
  ImGui::SameLine();
  ImGui::BeginGroup();
  
  ImGui::Text("EPA Steps");
  ImGui::Checkbox("ShowPenetrationVector", &collision->showPenetrationVector);
  ImGui::Checkbox("ShowClosestTriangle", &collision->showClosestTriangle);
  ImGui::Checkbox("HideMinkowskiRegion", &collision->hideMinkowskiRegion);
  ImGui::Checkbox("RenderSolidTriangles", &collision->renderSolidTriangles);
  ImGui::BeginChildFrame(1, ImVec2(400, 256));

  for (size_t i = 0; i < collision->epaSteps.count; i++) {
    char buffer[1024];
    sprintf(buffer, "%u", (U32)i);
    if (ImGui::Selectable(buffer, (editor->gjkStepIndex - collision->gjkSteps.count) == i)) {
      editor->gjkStepIndex = i + collision->gjkSteps.count;
    }
  }
  ImGui::EndChildFrame();
  ImGui::EndGroup();
}


static inline void DrawEditorGameTab(EditorData *editor) {

}