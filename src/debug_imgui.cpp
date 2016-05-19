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

static void
ShowDebugRenderSettings(VenomDebugRenderSettings* settings) {
	ImGui::Checkbox("isWireframeEnabled", &settings->isWireframeEnabled);
  ImGui::Checkbox("isDebugCameraActive", &settings->isDebugCameraActive);
  ImGui::Checkbox("disableCascadedShadowMaps", &settings->disableCascadedShadowMaps);
  ImGui::Checkbox("renderDebugNormals", &settings->renderDebugNormals);
  ImGui::Checkbox("renderFromDirectionalLight", &settings->renderFromDirectionalLight);
  ImGui::Checkbox("drawDepthMap", &settings->drawDepthMap);
}

#define GUI_U64(prefix, name) ImGui::Text(#name ": %lu", prefix->name)

void ShowVenomDebugRenderInfo(const VenomDebugRenderInfo* debugInfo) {
  GUI_U64(debugInfo, totalVerticesDrawn);
  GUI_U64(debugInfo, totalDrawCalls);
  GUI_U64(debugInfo, totalDrawListCommandsExecuted);
  GUI_U64(debugInfo, totalDrawListsRendered);
  ImGui::Text("directionaLightCount: %lu", debugInfo->directionalLightCount);
  ImGui::Text("pointLightCount: %lu", debugInfo->pointLightCount);
  ImGui::Text("shadowCastingPointLightCount: %lu", debugInfo->shadowCastingPointLightCount);
}

static void
ShowDebugInfo(GameMemory* memory) {
  ImGui::Begin("Venom Debug Info");
  ImGui::BeginGroup();
  ShowDebugRenderSettings(&memory->renderState.debugRenderSettings);
  ImGui::EndGroup();
  ImGui::SameLine();
  ImGui::BeginGroup();
  ShowVenomDebugRenderInfo(&memory->renderState.debugRenderInfo);
  ImGui::EndGroup();
  ImGui::End();
}

static void 
ShowProfiler(const ProfileData* profileData, const MemoryBlock* block) {
	ImGui::Begin("Profiler");
	ImGui::BeginGroup();
	ShowMemoryBlockTree(block);
	ImGui::EndGroup();
	ImGui::BeginGroup();
	for (U64 i = 0; i < profileData->persistantEntryCount; i++) {
		const PersistantProfilerEntry *entry = &profileData->persistantEntries[i];
		if (ImGui::CollapsingHeader(entry->name))
		{
			ImGui::Text("Elapsed Time: %f ms", 
          entry->elapsedTimeHistory[profileData->persistantWriteIndex]);
			//ImGui::Text("Elapsed Cycles: %llu", entry->elapsedCycles);
			ImGui::PushID(i);
			ImGui::PlotLines("", entry->elapsedTimeHistory, 
          PROFILER_ELAPSED_TIME_HISTORY_COUNT, profileData->persistantWriteIndex);
			ImGui::PopID();
		}
	}

	ImGui::EndGroup();
	ImGui::End();
}

#if 1
static void
ShowAssets(const GameAssets* assets) {
	ImGui::Begin("Assets");
	ImGui::Columns(2);
	ImGui::Separator();
	ImGui::Text("Materials");
	ImGui::NextColumn();
	ImGui::Text("Info");
	ImGui::NextColumn();
	ImGui::Separator();
	static int selected_index = -1;
	for (U64 i = 0; i < MaterialID_COUNT; i++) {
		const MaterialAsset *material = &assets->materialAssetList.materials[i];
		if (ImGui::Selectable(MATERIAL_NAMES[i], selected_index == (int)i))
			selected_index = i;
	}
	ImGui::NextColumn();

	if (selected_index >= 0) {
		const MaterialAsset *material = &assets->materialAssetList.materials[selected_index];

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
	ImGui::End();
}
#endif

static void
ShowConsole(const DebugLog *log) {
	ImGui::Begin("Console");
	ImGui::BeginGroup();
  
  for (size_t i = 0; i < log->current_entry_count; i++) {
		const LogEntry& entry = log->entries[i];
		const V4& color = LOGLEVEL_COLOR[entry.level];
		ImGui::TextColored(ImVec4(color.x, color.y, color.z, color.w), 
        "%s %s", LOGLEVEL_TAG[entry.level], log->entries[i].text);
	}

	ImGui::EndGroup();
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
