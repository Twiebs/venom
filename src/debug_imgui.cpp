#define ImGuiTextV3(v) ImGui::Text("%s: [%f, %f, %f]", #v, v.x, v.y, v.z)
#define ImGuiBoolEdit(b) ImGui::Checkbox(#b, &b)
#define ImGuiIntSlideEdit(i, min, max) ImGui::SliderInt(#i, &i, min, max);

static void ShowMemoryBlockTree(const MemoryBlock *block, U32 index = 0) {
	//NOTE(Torin) This should break with 2 levels of tree depth
	//but there is no reason the block hiarchy should ever be that way 
	float blockUsedPercentage = (int)(((float)block->used / (float)block->size) * 100);
	float blockSizeInMegabytes = (float)block->size / (float)MEGABYTES(1);
	float blockUsedInMegabytes = (float)block->used / (float)MEGABYTES(1);
	if ((ImGui::TreeNode((void*)(uintptr_t)index, "%s : (%.2f MB / %.2f MB) : %.1f%%", 
		block->name, blockUsedInMegabytes, blockSizeInMegabytes, blockUsedPercentage))) {
		for (U64 i = 0; i < block->childCount; i++) {
			ShowMemoryBlockTree(block->children[i], index + i);
		}
		ImGui::TreePop();
	}
}

#define GUI_U64(prefix, name) ImGui::Text(#name ": %lu", prefix->name)

static void 
draw_debug_render_info_ui(VenomDebugRenderFrameInfo* debugInfo, VenomDebugRenderSettings* renderSettings) {
  ImGui::BeginGroup();
  ImGui::Checkbox("isWireframeEnabled", &renderSettings->isWireframeEnabled);
  ImGui::Checkbox("isDebugCameraActive", &renderSettings->isDebugCameraActive);
  ImGui::Checkbox("disableCascadedShadowMaps", &renderSettings->disableCascadedShadowMaps);
  ImGui::Checkbox("disableAtmosphere", &renderSettings->disableAtmosphere);

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
draw_profiler_ui(const ProfileData* profileData, const MemoryBlock* block){
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

static bool ShowModelCreateWidgets(AssetManifest *manifest) {
  static char nameBuffer[256] = {};
  static char filenameBuffer[256] = {};

  if(ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer),
      ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll) || 
    ImGui::InputText("Filename", filenameBuffer, sizeof(filenameBuffer),
      ImGuiInputTextFlags_EnterReturnsTrue)) {

    if(nameBuffer[0] == 0) return 0;
    U32 slot_index = 0;
    if (manifest_contains_model(nameBuffer, manifest, &slot_index)) {
      LogWarning("Could not create model.  Asset named %s already exists!", nameBuffer);
      return true;
    }

    AssetSlot *slot = manifest->modelAssets.AddElement();
    slot->name = strdup(nameBuffer);
    slot->filename = strdup(filenameBuffer);
    memset(nameBuffer, 0, sizeof(nameBuffer));
    memset(filenameBuffer, 0, sizeof(filenameBuffer));
    return true;
  }

  return false;
}


//==================================================================================

static void ShowAssetSlotInfo(AssetSlot *slot, bool just_opened = false) {
  static char nameBuffer[256] = {};
  static char filenameBuffer[256] = {};
  if (just_opened == true) {
    memset(nameBuffer, 0x00, sizeof(nameBuffer));
    memset(filenameBuffer, 0x00, sizeof(filenameBuffer));
  }

  //NOTE(Torin) This will be problematic if the assset system trys to load
  //a file name that is in the process of beining modified

  if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
    free(slot->name);
    slot->name = strdup(nameBuffer);
  }
  
  if (ImGui::InputText("Filename", filenameBuffer, sizeof(filenameBuffer))) {
    free(slot->filename);
    slot->filename = strdup(filenameBuffer);
  }
}

static void ShowMaterialDataInfo(const MaterialData *data, bool firstOpen = false) {
  static bool isTextureVisible[MaterialTextureType_COUNT];
  if(firstOpen == true) {
    if(data->materialFlags & MaterialFlag_DIFFUSE)
      isTextureVisible[MaterialTextureType_DIFFUSE] = 1;
    if(data->materialFlags & MaterialFlag_SPECULAR)
      isTextureVisible[MaterialTextureType_SPECULAR] = 1;
    if(data->materialFlags & MaterialFlag_NORMAL)
      isTextureVisible[MaterialTextureType_NORMAL] = 1;
    if(data->materialFlags & MaterialFlag_SPECULAR)
      isTextureVisible[MaterialTextureType_SPECULAR] = 1;
  }

  ImGui::Text("MaterialSize: %dx%d", (int)data->textureWidth, (int)data->textureHeight);
  if(data->materialFlags & MaterialFlag_DIFFUSE)
    ImGui::Checkbox("Diffuse", &isTextureVisible[MaterialTextureType_DIFFUSE]);
  if(data->materialFlags & MaterialFlag_NORMAL)
    ImGui::Checkbox("Normal", &isTextureVisible[MaterialTextureType_NORMAL]);
  if(data->materialFlags & MaterialFlag_SPECULAR)
    ImGui::Checkbox("Specular", &isTextureVisible[MaterialTextureType_SPECULAR]);
}
//===================================================================================

static void
ShowConsole(const DebugLog *log, bool scrollToBottom = false){
	ImGui::Begin("Console");
	ImGui::BeginChild("LogEntries");
  for (size_t i = 0; i < log->current_entry_count; i++) {
		const LogEntry& entry = log->entries[i];
		const V4& color = LOGLEVEL_COLOR[entry.level];
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color.x, color.y, color.z, color.w));
    ImGui::TextWrapped("[%s %u:%u:%u] %s", LogLevelNames[entry.level],
      entry.time.hour, entry.time.minute, entry.time.second, entry.text);
    ImGui::PopStyleColor();
	}
  if (scrollToBottom) ImGui::SetScrollHere();
	ImGui::EndChild();
	ImGui::End();

  auto engine = GetEngine();
  engine->unseenErrorCount = 0;
  engine->unseenWarningCount = 0;
  engine->unseenInfoCount = 0;
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

static void draw_camera_info_ui(const Camera* camera) {
  ImGui::Text("Position: [%f, %f, %f]", camera->position.x, camera->position.y, camera->position.z);
  ImGui::Text("Front: [%f, %f, %f]", camera->front.x, camera->front.y, camera->front.z);
	ImGui::Text("Pitch: %f", RAD2DEG*camera->pitch);
	ImGui::Text("Yaw: %f", RAD2DEG*camera->yaw);
}