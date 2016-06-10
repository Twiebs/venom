#define ImGuiTextV3(v) ImGui::Text("%s: [%f, %f, %f]", #v, v.x, v.y, v.z)
#define ImGuiBoolEdit(b) ImGui::Checkbox(#b, &b)
#define ImGuiIntSlideEdit(i, min, max) ImGui::SliderInt(#i, &i, min, max);

static void ShowMemoryBlockTree(const MemoryBlock *block, U32 index = 0) {
	//NOTE(Torin) This should break with 2 levels of tree depth
	//but there is no reason the block hiarchy should ever be that way 
	float blockUsedPercentage = (int)(((float)block->used / (float)block->size) * 100);
	float blockSizeInMegabytes = (float)block->size / (float)MEGABYTES(1);
	float blockUsedInMegabytes = (float)block->used / (float)MEGABYTES(1);
	if ((ImGui::TreeNode((void*)index, "%s : (%.2f MB / %.2f MB) : %.1f%%", 
		block->name, blockUsedInMegabytes, blockSizeInMegabytes, blockUsedPercentage))) {
		for (U64 i = 0; i < block->childCount; i++) {
			ShowMemoryBlockTree(block->children[i], index + i);
		}
		ImGui::TreePop();
	}
}

#define GUI_U64(prefix, name) ImGui::Text(#name ": %lu", prefix->name)

static void 
ShowVenomDebugRenderInfo(VenomDebugRenderFrameInfo* debugInfo, 
    VenomDebugRenderSettings* renderSettings) 
{
  ImGui::BeginGroup();
  ImGui::Checkbox("isWireframeEnabled", &renderSettings->isWireframeEnabled);
  ImGui::Checkbox("isDebugCameraActive", &renderSettings->isDebugCameraActive);
  ImGui::Checkbox("disableCascadedShadowMaps", &renderSettings->disableCascadedShadowMaps);
  ImGui::Checkbox("renderDebugNormals", &renderSettings->renderDebugNormals);
  ImGui::Checkbox("drawPhysicsColliders", &renderSettings->drawPhysicsColliders);
  ImGui::Checkbox("renderFromDirectionalLight", 
    &renderSettings->renderFromDirectionalLight);
  ImGui::Checkbox("drawDepthMap", &renderSettings->drawDepthMap);
  ImGui::EndGroup();

  ImGui::SameLine();
  ImGui::BeginGroup();
  GUI_U64(debugInfo, totalVerticesDrawn);
  GUI_U64(debugInfo, totalDrawCalls);
  GUI_U64(debugInfo, totalDrawListCommandsExecuted);
  GUI_U64(debugInfo, totalDrawListsRendered);
  ImGui::Text("directionaLightCount: %lu", debugInfo->directionalLightCount);
  ImGui::Text("pointLightCount: %lu", debugInfo->pointLightCount);
  ImGui::Text("shadowCastingPointLightCount: %lu", 
    debugInfo->shadowCastingPointLightCount);
  ImGui::EndGroup();
}

static void 
ShowProfiler(const ProfileData* profileData, const MemoryBlock* block){
	for(size_t i = 0; i < profileData->persistantEntryCount; i++){
		const PersistantProfilerEntry *entry = &profileData->persistantEntries[i];
		if(ImGui::CollapsingHeader(entry->name)){
			ImGui::Text("Elapsed Time: %f ms", 
        entry->elapsedTimes[entry->historyWriteIndex]);
			ImGui::PushID(i);
			ImGui::PlotLines("", entry->elapsedTimes, 
        ARRAY_COUNT(entry->elapsedTimes), entry->historyWriteIndex); 
			ImGui::PopID();
		}
	}
}

static void
ShowDebugInfo(GameMemory* memory) {
  ImGui::Begin("Venom Debug Info");
  ShowVenomDebugRenderInfo(&memory->renderState.debugRenderFrameInfo, 
    &memory->renderState.debugRenderSettings);
  ImGui::BeginGroup();
  ShowProfiler(&memory->debugData.profileData, &memory->mainBlock);
  ImGui::EndGroup();
  ImGui::End();
}

static int 
ShowModelCreateWidgets(AssetManifest *manifest) {
  static char nameBuffer[256] = {};
  static char filenameBuffer[256] = {};

  if(ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer),
      ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll) || 
    ImGui::InputText("Filename", filenameBuffer, sizeof(filenameBuffer),
      ImGuiInputTextFlags_EnterReturnsTrue)) {

      if(nameBuffer[0] == 0) return 0;
      if(filenameBuffer[0] == 0) {
        //Create a new Model AssetFile 
      }

    AssetSlot slot = {};
    slot.name = strdup(nameBuffer);
    slot.filename = strdup(filenameBuffer);
    manifest->modelAssets.PushBack(slot);
    memset(nameBuffer, 0, sizeof(nameBuffer));
    memset(filenameBuffer, 0, sizeof(filenameBuffer));
    return 1;
  }

  return 0;
}

static void ShowMaterialDataInfo(const MaterialData *data, bool firstOpen = false) {
  static bool isTextureVisible[MaterialTextureType_COUNT];
  if(firstOpen == true) {
    if(data->flags & MaterialFlag_DIFFUSE) 
      isTextureVisible[MaterialTextureType_DIFFUSE] = 1;
    if(data->flags & MaterialFlag_SPECULAR) 
      isTextureVisible[MaterialTextureType_SPECULAR] = 1;
    if(data->flags & MaterialFlag_NORMAL) 
      isTextureVisible[MaterialTextureType_NORMAL] = 1;
    if(data->flags & MaterialFlag_SPECULAR) 
      isTextureVisible[MaterialTextureType_SPECULAR] = 1;
  }

  ImGui::Text("MaterialSize: %dx%d", (int)data->textureWidth, (int)data->textureHeight);
  if(data->flags & MaterialFlag_DIFFUSE) 
    ImGui::Checkbox("Diffuse", &isTextureVisible[MaterialTextureType_DIFFUSE]);
  if(data->flags & MaterialFlag_NORMAL) 
    ImGui::Checkbox("Diffuse", &isTextureVisible[MaterialTextureType_NORMAL]);
  if(data->flags & MaterialFlag_SPECULAR) 
    ImGui::Checkbox("Diffuse", &isTextureVisible[MaterialTextureType_SPECULAR]);
}

static void
ShowAssetManifest(AssetManifest* manifest){
	ImGui::Begin("Assets");
  if(ImGui::Button("SaveManifest")) {
    char temp[512];
    sprintf(temp, "assets%d.vsf", (int)time(0));
    WriteAssetManifestFile(temp, manifest);
    CopyFile(temp, "assets.vsf");
  }
	ImGui::Columns(2);
	ImGui::Separator();
  
  static U32 selectedAssetType = 0;
	if(ImGui::Button(AssetTypeNames[selectedAssetType]))
    ImGui::OpenPopup("AssetTypeSelect");
  if(ImGui::BeginPopup("AssetTypeSelect")) {
    for (size_t i = 0; i < AssetType_Count; i++) {
      if (ImGui::Selectable(AssetTypeNames[i])) {
        selectedAssetType= i;
      }
    }
    ImGui::EndPopup();
  }



	ImGui::NextColumn();
	ImGui::Text("Info");
	ImGui::NextColumn();
	ImGui::Separator();

  if(selectedAssetType == AssetType_Model){
    ImGui::BeginChild("AssetList");
    ImGui::Columns(2);
    ImGui::Text("Name");
    ImGui::NextColumn();
    ImGui::Text("Status");
    ImGui::NextColumn();
    ImGui::Separator();

    static int lastSelectedIndex = -1;
    static int selected_index = -1;
    for(U64 i = 0; i < manifest->modelAssets.count; i++){
      const AssetSlot* assetSlot = &manifest->modelAssets[i];
      if(ImGui::Selectable(assetSlot->name, selected_index == (int)i)){
        selected_index = i;
      }

      ImGui::NextColumn();
      static const ImColor loadedColor = ImColor(0.0f, 1.0f, 0.0f, 1.0f);
      static const ImColor unloadedColor = ImColor(1.0f, 0.0f, 0.0f, 1.0f);
      ImColor color = (assetSlot->flags & AssetFlag_Loaded) ? loadedColor : unloadedColor;
      const char *text = (assetSlot->flags & AssetFlag_Loaded) ? "loaded" : "unloaded";
      ImGui::TextColored(color, text);
      ImGui::NextColumn();
    }
    ImGui::EndChild();

    if(ImGui::Button("Create Model Asset"))
      ImGui::OpenPopup("NewModel"); 
    if(ImGui::Button("Destroy Model Asset")) {
      if(selected_index != -1) {
        DestroyModelAsset(selected_index, manifest);
      }
    }



    if(ImGui::BeginPopup("NewModel")){
      if(ShowModelCreateWidgets(manifest)) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    ImGui::NextColumn();
    if(selected_index != -1) {
      AssetSlot *slot = &manifest->modelAssets[selected_index];
      ModelAsset *asset = (ModelAsset *)(slot->asset);
      if(asset != 0) {
        static char nameBuffer[256] = {};
        static char filenameBuffer[256] = {};
        if(selected_index != lastSelectedIndex) {
          if(lastSelectedIndex != -1 && lastSelectedIndex != selected_index) {
            AssetSlot *lastSlot = &manifest->modelAssets[lastSelectedIndex];
            free(lastSlot->name);
            free(lastSlot->filename);
            lastSlot->name = strdup(nameBuffer);
            lastSlot->filename = strdup(filenameBuffer);
          }

          strcpy(nameBuffer, slot->name);
          strcpy(filenameBuffer, slot->filename);
          lastSelectedIndex = selected_index;
        }

        ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer));
        ImGui::InputText("Filename", filenameBuffer, sizeof(filenameBuffer));
        for(size_t i = 0; i < asset->data.meshCount; i++) {
          ShowMaterialDataInfo(&asset->data.materialDataPerMesh[i], 
            lastSelectedIndex != selected_index);
        }
      }
    }
  }

  else if(selectedAssetType == AssetType_Material){
    ImGui::BeginChild("AssetList");
    ImGui::Columns(2);
    ImGui::Text("Name");
    ImGui::NextColumn();
    ImGui::Text("Status");
    ImGui::NextColumn();
    ImGui::Separator();

    static int selected_index = -1;
    for(U64 i = 0; i < MaterialID_COUNT; i++){
      const MaterialAsset *material = &manifest->materialAssetList.materials[i];
      if(ImGui::Selectable(MATERIAL_NAMES[i], selected_index == (int)i)){
        selected_index = i;
      }

      ImGui::NextColumn();

      static const ImColor loadedColor = ImColor(0.0f, 1.0f, 0.0f, 1.0f);
      static const ImColor unloadedColor = ImColor(1.0f, 0.0f, 0.0f, 1.0f);
      ImColor color = (material->data.textureData != 0) ? loadedColor : unloadedColor;
      const char *text = (material->data.textureData != 0) ? "loaded" : "unloaded";
      ImGui::TextColored(color, text);
      ImGui::NextColumn();
    }
    ImGui::EndChild();
    ImGui::NextColumn();

    if(selected_index >= 0) {
      const MaterialAsset *material = &manifest->materialAssetList.materials[selected_index];

      static const U32 textureDisplaySize = 256;
      ImVec2 textureBounds = ImVec2(textureDisplaySize, textureDisplaySize);
      ImGui::BeginChild("diffuse", ImVec2(textureDisplaySize, textureDisplaySize + 18));
      ImGui::Text("Diffuse Texture");
      ImGui::Image((ImTextureID)material->drawable.diffuse_texture_id, textureBounds);
      ImGui::EndChild();

      if (material->data.flags & MaterialFlag_NORMAL) {
        ImGui::SameLine();
        ImGui::BeginChild("normal", ImVec2(textureDisplaySize, textureDisplaySize + 18));
        ImGui::Text("Normal Texture");
        ImGui::Image((ImTextureID)material->drawable.normal_texture_id, textureBounds);
        ImGui::EndChild();
      }

      if (material->data.flags & MaterialFlag_SPECULAR) {
        ImGui::SameLine();
        ImGui::BeginChild("specular", ImVec2(textureDisplaySize, textureDisplaySize + 18));
        ImGui::Text("Specular Texture");
        ImGui::Image((ImTextureID)material->drawable.specular_texture_id, textureBounds);
        ImGui::EndChild();
      }

    }
    ImGui::NextColumn();


  }
  	ImGui::End();
}

static void
ShowEventOverlay(const VenomDebugData& data){
  static const U32 overlayWidth = 250;
  static const U32 overlayPadding = 8;
  const ImGuiIO& io = ImGui::GetIO();
  const U32 xpos = io.DisplaySize.x - overlayWidth - overlayPadding;
  ImGui::SetNextWindowPos(ImVec2(xpos, overlayPadding));
  ImGui::SetNextWindowSize(ImVec2(overlayWidth, 0));
  ImGui::Begin("EventOverlay", 0, ImVec2(0,0), 0.3f, 
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings); 
  if(data.unseenErrorCount > 0)
    ImGui::TextColored(ImColor(255,0,0),"Errors: %u", data.unseenErrorCount);
  if(data.unseenWarningCount > 0)
    ImGui::TextColored(ImColor(255,255,0),"Warnings: %u", data.unseenWarningCount);
  ImGui::End();
}

static void
ShowConsole(const DebugLog *log, bool scrollToBottom = false){
	ImGui::Begin("Console");
	ImGui::BeginChild("LogEntries");
  for (size_t i = 0; i < log->current_entry_count; i++) {
		const LogEntry& entry = log->entries[i];
		const V4& color = LOGLEVEL_COLOR[entry.level];
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color.x, color.y, color.z, color.w));
		ImGui::TextWrapped("%s %s", LOGLEVEL_TAG[entry.level], log->entries[i].text);
    ImGui::PopStyleColor();
	}
  if (scrollToBottom) ImGui::SetScrollHere();
	ImGui::EndChild();
	ImGui::End();
}

static void 
ShowInputState(const InputState* input) {
	ImGui::Begin("InputState");
	ImGui::Text("CursorX: %d", input->cursorPosX);
	ImGui::Text("CursorY: %d", input->cursorPosY);
	ImGui::Text("CursorDeltaX: %d", input->cursorDeltaX);
	ImGui::Text("CursorDeltaY: %d", input->cursorDeltaY);
	ImGui::Text("LeftButtonDown: %s", input->isButtonDown[MOUSE_LEFT] ? "TRUE" : "FALSE");
	ImGui::Text("RightButtonDown: %s", input->isButtonDown[MOUSE_RIGHT] ? "TRUE" : "FALSE");
	ImGui::End();
}

static void
ShowSystemInfo(const SystemInfo* sys) {
	F64 ramInGigabytes = (F64)sys->virtual_memory_size / (1L << 30);
	ImGui::Begin("SystemInfo");
	ImGui::Text("SystemPhysicalMemory: %f", ramInGigabytes);
	ImGui::End();
}

static void
ShowCameraInfo(const Camera* camera) {
	ImGui::Begin("CameraInfo");
	ImGuiTextV3(camera->position);
	ImGuiTextV3(camera->front);
	ImGui::Text("Pitch: %f", RAD2DEG*camera->pitch);
	ImGui::Text("Yaw: %f", RAD2DEG*camera->yaw);
	ImGui::End();
}
