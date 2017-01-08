#pragma clang diagnostic error "-Wall"
#pragma clang diagnostic error "-Wextra"
#pragma clang diagnostic error "-Wreturn-type" 
#pragma clang diagnostic error "-Wunused-value"
#pragma clang diagnostic error "-Wunknown-pragmas"
#pragma clang diagnostic error "-Wparentheses"
#pragma clang diagnostic error "-Wformat-extra-args"
#pragma clang diagnostic warning "-Wall"
#pragma clang diagnostic warning "-Wextra"
#pragma clang diagnostic ignored "-Wformat-security"

#if !defined(VENOM_RELEASE) && !defined(VENOM_STRICT)
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wswitch"
#endif//!defined(VENOM_RELEASE) && !defined(VENOM_STRICT)

#if defined(VENOM_HOTLOAD) || 0
#include "venom_platform.h"
#define _(returnType, name, ...) static name##Proc name;
EngineAPIList
#undef _

#ifdef VENOM_SINGLE_TRANSLATION_UNIT
#include "venom_render.cpp"
#include "venom_asset.cpp"
#include "venom_physics.cpp"
#include "venom_entity.cpp"
#include "math_procedural.cpp"
#ifndef VENOM_RELEASE
#include "venom_debug.cpp"
#include "venom_editor.cpp"
#include "venom_serializer.cpp"
#endif//VENOM_RELEASE
#endif//VENOM_SINGE_TRANSLATION_UNIT
#endif//VENOM_HOTLOAD

//TODO(Torin) Remove This!!!
static
int VenomCopyFile(const char *a, const char *b) {
  FILE *fa = fopen(a, "rb");
  FILE *fb = fopen(b, "wb");
  if(fa == 0 || fb == 0) return 0;
  fseek(fa, 0, SEEK_END);
  size_t fileSize = ftell(fa);
  fseek(fa, 0, SEEK_SET);
  void *buffer = malloc(fileSize);
  fread(buffer, 1, fileSize, fa);
  fwrite(buffer, 1, fileSize, fb);
  free(buffer);
  fclose(fa);
  fclose(fb);
  return 1;
}

#include "game.h"

void VenomModuleStart(GameMemory* memory);
void VenomModuleLoad(GameMemory* memory);
void VenomModuleUpdate(GameMemory* memory);
void VenomModuleRender(GameMemory* memory);

#if 1
#define _(name, flags) MaterialID_##name,
enum MaterialID { 
#ifdef VENOM_MATERIAL_LIST_FILE
#include VENOM_MATERIAL_LIST_FILE
#endif//VENOM_MATERIAL_LIST_FILE
  MaterialID_COUNT 
};
#undef _

#define _(name, flags) #name,
const char *MATERIAL_NAMES[] = { 
#ifdef VENOM_MATERIAL_LIST_FILE
#include VENOM_MATERIAL_LIST_FILE
#endif//VENOM_MATERIAL_LIST_FILE
  "FUCKYOUWINDOWS"
}; 
#undef _
#endif
#include "debug_imgui.cpp"

static inline
void LoadMaterialList(MaterialAssetList* list) {
  if(list->materialCount != MaterialID_COUNT){
    for(U64 i = 0; i < list->materialCount; i++){
      MaterialAsset& material = list->materials[i];
      if(material.data.textureData != nullptr){
        DestroyMaterialData(&material.data);
      }
      if(material.drawable.diffuse_texture_id != 0){
        DestroyMaterialDrawable(&material.drawable);
      }
    }

    if (list->materials != nullptr)
      free(list->materials);
    list->materials = (MaterialAsset*)malloc(sizeof(MaterialAsset) * MaterialID_COUNT);
    memset(list->materials, 0, sizeof(MaterialAsset) * MaterialID_COUNT);
    list->materialCount = MaterialID_COUNT;

#define _(name, f) \
    list->materials[MaterialID_##name].flags = (f); \
    if ((f) & MaterialFlag_DIFFUSE) \
      list->materials[MaterialID_##name].filenames[MaterialTextureType_DIFFUSE] = \
        VENOM_ASSET_FILE("materials/" #name "/diffuse.tga");\
    if ((f) & MaterialFlag_NORMAL)\
      list->materials[MaterialID_##name].filenames[MaterialTextureType_NORMAL] =\
        VENOM_ASSET_FILE("materials/" #name "/normal.tga");\
    if ((f) & MaterialFlag_SPECULAR)\
      list->materials[MaterialID_##name].filenames[MaterialTextureType_SPECULAR] =\
        VENOM_ASSET_FILE("materials/" #name "/specular.tga");
#ifdef VENOM_MATERIAL_LIST_FILE
#include VENOM_MATERIAL_LIST_FILE 
#endif//VENOM_MATERIAL_LIST_FILE
#undef _
  }
}

static inline
void InitalizeVenomDebugData(GameMemory* memory) {
  RenderState* rs = &memory->renderState;
  InitializeCamera(&rs->debugCamera, 45.0f * DEG2RAD, 0.01f, 256.0f,
    memory->systemInfo.screen_width, memory->systemInfo.screen_height);
  rs->debugCamera.position = V3(0.0f, 4.0f, 0.0f);
  rs->debugCamera.front = V3(-0.7f, 0.0f, 0.7f);
  rs->imguiRenderGroup = CreateImGuiRenderGroup();
}

static void 
RenderImGuiDrawList(ImDrawData *data) {
	ImGuiIO& io = ImGui::GetIO();
	GameMemory *memory = (GameMemory*)io.UserData;
	RenderState *rs = &memory->renderState;
	SystemInfo *system = &memory->systemInfo;
	memory->renderState.imgui_draw_data = data;

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(0, 0, system->screen_width, system->screen_height);
	glUseProgram(GetShaderProgram(ShaderID_Sprite, &memory->assetManifest));
	M4 screenProjection = Orthographic(0.0f, system->screen_width, 
    0.0f, system->screen_height, -1.0f, 100.0f, -1.0f);
	glUniformMatrix4fv(2, 1, GL_FALSE, &screenProjection[0][0]);
  glBindVertexArray(rs->imguiRenderGroup.vao);
  glActiveTexture(GL_TEXTURE0);

  ImDrawData *draw_data = rs->imgui_draw_data;
  for (int i = 0; i < rs->imgui_draw_data->CmdListsCount; i++) {
    const ImDrawList *draw_list = draw_data->CmdLists[i];
    const ImDrawIdx *index_buffer_offset = 0;
    glBindBuffer(GL_ARRAY_BUFFER, rs->imguiRenderGroup.vbo);
    glBufferData(GL_ARRAY_BUFFER, draw_list->VtxBuffer.size() * sizeof(ImDrawVert),
      &draw_list->VtxBuffer.front(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rs->imguiRenderGroup.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, draw_list->IdxBuffer.size() * sizeof(ImDrawIdx),
      &draw_list->IdxBuffer.Data[0], GL_DYNAMIC_DRAW);

    for (const ImDrawCmd* cmd = draw_list->CmdBuffer.begin(); 
          cmd != draw_list->CmdBuffer.end(); cmd++) {
      glBindTexture(GL_TEXTURE_2D, (GLuint)(uintptr_t)cmd->TextureId);
      glDrawElements(GL_TRIANGLES, cmd->ElemCount, GL_UNSIGNED_SHORT, index_buffer_offset);
      index_buffer_offset += cmd->ElemCount;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

	glDisable(GL_SCISSOR_TEST);
	glUseProgram(0);
}

static inline void 
imgui_update_state(GameMemory *memory){
	InputState *input = &memory->inputState;
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = memory->deltaTime;

	//TODO(Torin) Figure out why windows tells us the cursor is higher
#ifdef _WIN32
	io.MousePos = ImVec2((float)input->cursorPosX, (float)input->cursorPosY + 40);
#else
	io.MousePos = ImVec2((float)input->cursorPosX, (float)input->cursorPosY);
#endif
	io.MouseDelta = ImVec2((float)input->cursorDeltaX, (float)input->cursorDeltaY);
	io.MouseDown[0] = input->isButtonDown[MOUSE_LEFT];
	io.MouseDown[1] = input->isButtonDown[MOUSE_RIGHT];
	io.MouseDown[2] = input->isButtonDown[MOUSE_MIDDLE];
	memcpy(io.KeysDown, input->isKeyDown, 256);
}


extern "C" 
void _VenomModuleStart(GameMemory* memory) {
#ifdef VENOM_HOTLOAD
#define _(returnType, name, ...) name = memory->engineAPI.name;
EngineAPIList
#undef _
#define _(signature, name) name = memory->engineAPI.name;
#include "opengl_procedures.h"
#undef _ 
#endif//VENOM_HOTLOAD

  InitalizeVenomDebugData(memory);
  InitalizeRenderState(&memory->renderState, &memory->systemInfo);

  U8* pixels;
  int width, height;
  ImGuiIO& io = ImGui::GetIO();
  //io.Fonts->AddFontFromFileTTF("/usr/share/fonts/TTF/DejaVuSans.ttf", 14);
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  GLuint textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
    width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
  memory->renderState.imguiFontTexture = textureID;
  io.Fonts->TexID = (void *)(size_t)memory->renderState.imguiFontTexture;


  initalize_asset_manifest(&memory->assetManifest);

  VenomModuleStart(memory);

#if 0
  assert(memory->mainBlock.size > memory->mainBlock.used);
  U64 remainingBlockMemory = memory->mainBlock.size - memory->mainBlock.used;
  InitSubBlock("AssetDataMemory", &memory->assetManifest.memoryBlock, remainingBlockMemory, &memory->mainBlock);
#endif

  
  
  LoadMaterialList(&memory->assetManifest.materialAssetList);

#if 0
  //NOTE(Torin) Hack to import assets into the new system from the old system
  static const char *MODEL_ASSET_FILENAMES[] = {
#define _(name, file) VENOM_ASSET_FILE(file), 
    DebugModelList
#undef _ 
  };

  static const char* MODEL_ASSET_NAMES[] = {
#define _(name,file) #name,
    DebugModelList
#undef _
  };

enum DEBUGModelID {
#define _(name, file) DEBUGModelID_##name, 
	DebugModelList
#undef _ 
	DEBUGModelID_COUNT
};

  AssetManifest* manifest = &memory->assetManifest;
  for(size_t i = 0; i < DEBUGModelID_COUNT; i++){
    AssetSlot slot = {};
    slot.name = MODEL_ASSET_NAMES[i];
    slot.filename = MODEL_ASSET_FILENAMES[i];
    manifest->modelAssets.push_back(slot);
  }
#endif
}

#if 0
void LoadAssetList(AssetList* list, AssetType type, U32 expectedCount){
  if(list->assetCount != expectedCount){
    for(U64 i = 0; i < list->assetCount; i++){
      AssetSlot* slot = &list->slots[i];
      if(slot->flags & AssetFlag_Loaded){
        DestroyAsset...
      }
    }
    
    if(list->slots != nullptr) free(list->slots);
    list->slots = (AssetSlot*)calloc(sizeof(AssetSlot)*expectedCount, 1);
    list->assetCount = expectedCount;
  }
}
#endif

//TODO(Torin) Real keyboard handling system
static void
HackVenomKeyEventCallback(int keycode, int keysym, int keystate){
  ImGuiIO& io = ImGui::GetIO();
  if(keystate == 1){
    if(keysym >= 32 && keysym <= 127) {
      io.AddInputCharacter(keysym);
    }
  }

  switch (keycode) {
  case KEYCODE_F2: {

  } break;


  }

}


extern "C" void _VenomModuleLoad(GameMemory *memory) {
#ifdef VENOM_HOTLOAD
#define _(returnType, name, ...) name = memory->engineAPI.name;
EngineAPIList
#undef _
#define _(signature, name) name = memory->engineAPI.name;
#include "opengl_procedures.h"
#undef _ 
#endif//VENOM_HOTLOAD
	
	ImGuiIO& io = ImGui::GetIO();
  
	SystemInfo *system = &memory->systemInfo;
  io.KeyMap[ImGuiKey_Enter] = KEYCODE_ENTER;
	io.KeyMap[ImGuiKey_Escape] = KEYCODE_ESCAPE;
	io.KeyMap[ImGuiKey_Tab] = KEYCODE_TAB;
	io.KeyMap[ImGuiKey_Backspace] = KEYCODE_BACKSPACE;
	io.KeyMap[ImGuiKey_LeftArrow] = KEYCODE_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = KEYCODE_RIGHT;
	io.KeyMap[ImGuiKey_DownArrow] = KEYCODE_DOWN;
	io.KeyMap[ImGuiKey_UpArrow] = KEYCODE_UP;

  io.KeyMap[ImGuiKey_A] = KEYCODE_A;
  io.KeyMap[ImGuiKey_C] = KEYCODE_C;
  io.KeyMap[ImGuiKey_V] = KEYCODE_V;
  io.KeyMap[ImGuiKey_X] = KEYCODE_X;
  io.KeyMap[ImGuiKey_Y] = KEYCODE_Y; 
  io.KeyMap[ImGuiKey_Z] = KEYCODE_Z; 
  memory->keyEventCallback = HackVenomKeyEventCallback;

	io.RenderDrawListsFn = RenderImGuiDrawList;
	io.DisplaySize = ImVec2(system->screen_width, system->screen_height);
	io.UserData = memory;

  U8* pixels;
  int width, height, components;
  //io.Fonts->AddFontFromFileTTF("/usr/share/fonts/TTF/DejaVuSans.ttf", 14);
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &components);
	io.Fonts->TexID = (void *)(size_t)memory->renderState.imguiFontTexture;

#if 0
  { //Reload the MaterialList Why the hell are we doing this?
    MaterialAssetList* list = &memory->assets.materialAssetList;
    LoadMaterialList(list);
  }
#endif

  InitalizeEditor(&memory->editor);
  VenomModuleLoad(memory);
}

extern "C" void _VenomModuleUpdate(GameMemory* memory) {
  BeginTimedBlock("HotloadAssets");
  hotload_modified_assets(&memory->assetManifest);
  EndTimedBlock("HotloadAssets");

  imgui_update_state(memory);
  ImGui::NewFrame();

  BeginTimedBlock("Update");
  VenomModuleUpdate(memory);
  EndTimedBlock("Update");
  FinalizeAllTasks(GetEngine());
}

extern "C"
void _VenomModuleRender(GameMemory* memory) {
  BeginTimedBlock("GPU Submit");
  memset(&memory->renderState.debugRenderFrameInfo, 0, sizeof(VenomDebugRenderFrameInfo));
  VenomModuleRender(memory);
  ImGui::Render();
  EndTimedBlock("GPU Submit");
  Memory::FrameStackClear();
}
