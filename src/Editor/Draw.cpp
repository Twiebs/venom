
#define IMGUI_U64(prefix, name) ImGui::Text(#name ": %lu", prefix->name)

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

static inline void ImGuiSetStyle() {
  ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 0.85f));

  ImVec4 darkColor = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
  ImVec4 darkColorHover = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);

  ImGui::PushStyleColor(ImGuiCol_Button, darkColor);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, darkColorHover);

  ImGui::PushStyleColor(ImGuiCol_Header, darkColor);
  ImGui::PushStyleColor(ImGuiCol_HeaderActive, darkColor);
  ImGui::PushStyleColor(ImGuiCol_HeaderHovered, darkColorHover);

  ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, darkColor);
  ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

  ImGui::PushStyleColor(ImGuiCol_PlotHistogram, darkColor);
}



static inline void DrawProfiler(ProfileData* profileData) {
  const char *profileStateText = profileData->isPaused ? "RESUME" : "PAUSE";
  if (ImGui::Button(profileStateText)) {
    profileData->isPaused = !profileData->isPaused;
  }

  ImGui::Separator();

  static const F32 HISTOGRAM_WIDTH = 720.0f;
  ImGui::BeginChildFrame(0, ImVec2(HISTOGRAM_WIDTH + 32, 0));
  for (size_t i = 0; i < profileData->persistantEntryCount; i++) {
    const PersistantProfilerEntry *entry = &profileData->persistantEntries[i];
    if (ImGui::CollapsingHeader(entry->name, ImGuiTreeNodeFlags_DefaultOpen)) {
      //ImGui::Text("Elapsed Time: %.4f ms", entry->elapsedTimes[entry->historyWriteIndex]);
      ImGui::PushID(i);
      //ImGui::SameLine();
      ImGui::PlotHistogram("", entry->elapsedTimes, 128, entry->historyWriteIndex, 0, 0.00f, 16.0f, ImVec2(HISTOGRAM_WIDTH, 60));
      //ImGui::PlotLines("", entry->elapsedTimes, ARRAY_COUNT(entry->elapsedTimes), entry->historyWriteIndex);
      ImGui::PopID();
    }
  }
  ImGui::EndChildFrame();
  ImGui::SameLine();
  
  ImGui::BeginChildFrame(1, ImVec2(0, 0));
  for (size_t i = 0; i < profileData->explicitEntryCount; i++) {
    ExplicitProfilerEntry *entry = &profileData->explictEntries[i];
    ImGui::Text("%s: %.2f ms", entry->name, entry->elapsedTimeMilliseconds);
  }
  ImGui::EndChildFrame();
}

static inline void DrawConsole(const DebugLog *log, bool scrollToBottom = false) {
  auto engine = GetEngine();
  ImGui::Begin("Console");
  ImGui::BeginChildFrame(0, ImVec2(480, 0));
  for (size_t i = 0; i < log->current_entry_count; i++) {
    const LogEntry& entry = log->entries[i];
    const V4& color = LOGLEVEL_COLOR[entry.level];
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color.x, color.y, color.z, color.w));
    ImGui::TextWrapped("[%s %u:%u:%u] %s", LogLevelNames[entry.level],
      entry.time.hour, entry.time.minute, entry.time.second, entry.text);
    ImGui::PopStyleColor();
  }
  if (scrollToBottom) ImGui::SetScrollHere();
  ImGui::EndChildFrame();
  ImGui::SameLine();
  ImGui::BeginChildFrame(1, ImVec2(0, 0));
  DrawProfiler(&engine->profileData);
  ImGui::EndChildFrame();
  ImGui::End();

  engine->unseenErrorCount = 0;
  engine->unseenWarningCount = 0;
  engine->unseenInfoCount = 0;
}

static void DrawAssetSlotInfo(AssetSlot *slot, bool just_opened = false) {
  static char nameBuffer[256] = {};
  static char filenameBuffer[256] = {};
  if (just_opened == true) {
    memset(nameBuffer, 0x00, sizeof(nameBuffer));
    memset(filenameBuffer, 0x00, sizeof(filenameBuffer));
  }

  //NOTE(Torin) This will be problematic if the assset system trys to load
  //a file name that is in the process of beining modified
  if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
    MemoryFree(slot->name);
    slot->name = CStringDuplicate(nameBuffer);
  }

  if (ImGui::InputText("Filename", filenameBuffer, sizeof(filenameBuffer))) {
    MemoryFree(slot->filename);
    slot->filename = CStringDuplicate(filenameBuffer);
  }
}



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

static bool ShowModelCreateWidgets(AssetManifest *manifest) {
  static char nameBuffer[256] = {};
  static char filenameBuffer[256] = {};

  if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer),
    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll) ||
    ImGui::InputText("Filename", filenameBuffer, sizeof(filenameBuffer),
      ImGuiInputTextFlags_EnterReturnsTrue)) {

    if (nameBuffer[0] == 0) return 0;
    U32 slot_index = 0;
    if (manifest_contains_model(nameBuffer, manifest, &slot_index)) {
      LogWarning("Could not create model.  Asset named %s already exists!", nameBuffer);
      return true;
    }

    AssetSlot *slot = manifest->modelAssets.AddElement();
    slot->name = CStringDuplicate(nameBuffer);
    slot->filename = CStringDuplicate(filenameBuffer);
    memset(nameBuffer, 0, sizeof(nameBuffer));
    memset(filenameBuffer, 0, sizeof(filenameBuffer));
    return true;
  }

  return false;
}

static void ShowMaterialDataInfo(const MaterialData *data, bool firstOpen = false) {
  static bool isTextureVisible[MaterialTextureType_COUNT];
  if (firstOpen == true) {
    if (data->materialFlags & MaterialFlag_DIFFUSE)
      isTextureVisible[MaterialTextureType_DIFFUSE] = 1;
    if (data->materialFlags & MaterialFlag_SPECULAR)
      isTextureVisible[MaterialTextureType_SPECULAR] = 1;
    if (data->materialFlags & MaterialFlag_NORMAL)
      isTextureVisible[MaterialTextureType_NORMAL] = 1;
    if (data->materialFlags & MaterialFlag_SPECULAR)
      isTextureVisible[MaterialTextureType_SPECULAR] = 1;
  }

  ImGui::Text("MaterialSize: %dx%d", (int)data->textureWidth, (int)data->textureHeight);
  if (data->materialFlags & MaterialFlag_DIFFUSE)
    ImGui::Checkbox("Diffuse", &isTextureVisible[MaterialTextureType_DIFFUSE]);
  if (data->materialFlags & MaterialFlag_NORMAL)
    ImGui::Checkbox("Normal", &isTextureVisible[MaterialTextureType_NORMAL]);
  if (data->materialFlags & MaterialFlag_SPECULAR)
    ImGui::Checkbox("Specular", &isTextureVisible[MaterialTextureType_SPECULAR]);
}

//===================================================================================

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
  F64 ramInGigabytes = (F64)sys->virtualMemorySize / (1L << 30);
  ImGui::Begin("SystemInfo");
  ImGui::Text("SystemPhysicalMemory: %f", ramInGigabytes);
  ImGui::End();
}



static inline void DrawVisualizationWindow(EditorData *editor) {
  U32 textureID = 0;

  editor->visualizerMode = VisualizerMode_VisualizerScene;
  switch (editor->visualizerMode) {

  case VisualizerMode_GBufferColor: {
    auto rs = &GetVenomEngineData()->renderState;
    textureID = rs->gbuffer.albedoSpecular;
  } break;

  case VisualizerMode_GBufferNormal: {
    auto rs = &GetVenomEngineData()->renderState;
    textureID = rs->gbuffer.normal;
  } break;

  case VisualizerMode_VisualizerScene: {
    textureID = editor->visualizerColorBuffer;
  } break;

  }


  ImGui::SetNextWindowPos(ImVec2(5.0f, 45.0f), ImGuiSetCond_Always);
  ImGui::Begin("Visualizer", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);
  ImGui::Image((ImTextureID)textureID, ImVec2(editor->visualizerWidth, editor->visualizerHeight));
  if (ImGui::IsItemClicked(1)) {
    editor->isVisualizerActive = true;
  }

  ImGui::End();

}

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