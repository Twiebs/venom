#pragma clang diagnostic warning "-Wall"
#pragma clang diagnostic warning "-Wextra"
#pragma clang diagnostic ignored "-Wformat-security"

#if !defined(VENOM_RELEASE) && !defined(VENOM_STRICT)
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#pragma clang diagnostic ignored "-Wunused-variable"
#endif



#ifdef VENOM_HOTLOAD
#include "venom_platform.h"
#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_demo.cpp"
#include "venom_render.cpp"

#include "game.h"

#define _(returnType, name, ...) static name##Proc name;
EngineAPIList
#undef _
#endif//VENOM_HOTLOAD

void VenomModuleStart(GameMemory* memory);
void VenomModuleLoad(GameMemory* memory);
void VenomModuleUpdate(GameMemory* memory);
void VenomModuleRender(GameMemory* memory);

#define _(name, flags) MaterialID_##name,
enum MaterialID { 
#ifdef VENOM_MATERIAL_LIST_FILE
#include VENOM_MATERIAL_LIST_FILE
#endif//VENOM_MATERIAL_LIST_FILE
  MaterialID_COUNT 
};
#undef _

#define _(name, flags) #name,
const char *MATERIAL_NAMES[] { 
#ifdef VENOM_MATERIAL_LIST_FILE
#include VENOM_MATERIAL_LIST_FILE
#endif//VENOM_MATERIAL_LIST_FILE
}; 
#undef _

#include "debug_imgui.cpp"
#include "game.h"

static inline
void InitalizeVenomDebugData(GameMemory* memory) {
  RenderState* rs = &memory->renderState;
  GameAssets* assets = &memory->assets;
  InitSubBlock("vertex_block", &rs->vertexBlock, MEGABYTES(128), &memory->mainBlock);
  InitSubBlock("intex_block", &rs->indexBlock, MEGABYTES(128), &memory->mainBlock);
  InitializeCamera(&rs->debugCamera, 45.0f * DEG2RAD, 0.1f, 256.0f,
    memory->systemInfo.screen_width, memory->systemInfo.screen_height);
  rs->debugCamera.position = V3(0.0f, 4.0f, 0.0f);
  rs->debugCamera.front = V3(-0.7f, 0.0f, 0.7f);

  rs->imguiRenderGroup = CreateImGuiRenderGroup();
  rs->solidDebugGroup = CreateDebugRenderGroup();
  rs->lineDebugGroup = CreateDebugRenderGroup();

  rs->debugShader = GetShaderProgram(ShaderID_Debug, assets);
  rs->singleColorShader = GetShaderProgram(ShaderID_SingleColor, assets);
  rs->imguiRenderGroupShader = GetShaderProgram(ShaderID_Sprite, assets);
  rs->debugNormalsShader = GetShaderProgram(ShaderID_debug_normals, assets);
  rs->debug_depth_map_shader = GetShaderProgram(ShaderID_debug_depth_map, assets);
}

static inline
void InitalizeRenderState(RenderState* rs) {
  glGenBuffers(1, &rs->quadVao);
}

static void imgui_render_draw_lists(ImDrawData *data)
{
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
	glUseProgram(rs->imguiRenderGroupShader);

	M4 screenProjection = Orthographic(0.0f, system->screen_width, 
    0.0f, system->screen_height, -1.0f, 100.0f, -1.0f);

	glUniformMatrix4fv(2, 1, GL_FALSE, &screenProjection[0][0]);

	if (rs->imgui_draw_data != nullptr)
	{
		glBindVertexArray(rs->imguiRenderGroup.vao);
		glActiveTexture(GL_TEXTURE0);
		ImDrawData *draw_data = rs->imgui_draw_data;
		for (int i = 0; i < rs->imgui_draw_data->CmdListsCount; i++)
		{
			const ImDrawList *draw_list = draw_data->CmdLists[i];
			const ImDrawIdx *index_buffer_offset = 0;
			glBindBuffer(GL_ARRAY_BUFFER, rs->imguiRenderGroup.vbo);
			glBufferData(GL_ARRAY_BUFFER, draw_list->VtxBuffer.size() * sizeof(ImDrawVert),
				&draw_list->VtxBuffer.front(), GL_DYNAMIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rs->imguiRenderGroup.ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, draw_list->IdxBuffer.size() * sizeof(ImDrawIdx),
				&draw_list->IdxBuffer.Data[0], GL_DYNAMIC_DRAW);

			for (const ImDrawCmd* cmd = draw_list->CmdBuffer.begin(); cmd != draw_list->CmdBuffer.end(); cmd++)
			{
				glBindTexture(GL_TEXTURE_2D, (GLuint)(uintptr_t)cmd->TextureId);
				glDrawElements(GL_TRIANGLES, cmd->ElemCount, GL_UNSIGNED_SHORT, index_buffer_offset);
				index_buffer_offset += cmd->ElemCount;
			}
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
		rs->imgui_draw_data = nullptr;
	}

	glDisable(GL_SCISSOR_TEST);
	glUseProgram(0);
}

static inline void 
imgui_update_state(GameMemory *memory) {
	InputState *input = &memory->inputState;
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = memory->gameState.deltaTime;
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
  InitalizeRenderState(&memory->renderState);

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
  VenomModuleStart(memory);

  assert(memory->mainBlock.size > memory->mainBlock.used);
  U64 remainingBlockMemory = memory->mainBlock.size - memory->mainBlock.used;
  InitSubBlock("AssetCache", &memory->assets.memory,
    remainingBlockMemory, &memory->mainBlock);
}

void LoadMaterialList(MaterialAssetList* list) {
  if (list->materialCount != MaterialID_COUNT) {
    for (U64 i = 0; i < list->materialCount; i++) {
      MaterialAsset& material = list->materials[i];
      if (material.data.textureData != nullptr) {
        DestroyMaterialData(&material.data);
      }
      if (material.drawable.diffuse_texture_id != 0) {
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

	io.RenderDrawListsFn = imgui_render_draw_lists;
	io.DisplaySize = ImVec2(system->screen_width, system->screen_height);
	io.UserData = memory;

  U8* pixels;
  int width, height, components;
  //io.Fonts->AddFontFromFileTTF("/usr/share/fonts/TTF/DejaVuSans.ttf", 14);
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &components);
	io.Fonts->TexID = (void *)(size_t)memory->renderState.imguiFontTexture;

  { //Reload the MaterialList
    MaterialAssetList* list = &memory->assets.materialAssetList;
    LoadMaterialList(list);
  }

  VenomModuleLoad(memory);
}

extern "C" void _VenomModuleUpdate(GameMemory* memory) {
  imgui_update_state(memory);
  ImGui::NewFrame();
  VenomModuleUpdate(memory);
  ShowConsole(&memory->debug_memory.debugLog);
  ShowDebugInfo(memory);

  InputState* input = &memory->inputState;
  if (input->isButtonDown[MOUSE_RIGHT]) {
    MoveCameraWithFPSControls(&memory->renderState.debugCamera,
      &memory->inputState, memory->gameState.deltaTime);
  }
  ShowCameraInfo(&memory->renderState.debugCamera);
  //ImGui::ShowTestWindow();
}

extern "C"
void _VenomModuleRender(GameMemory* memory) {
  memset(&memory->renderState.debugRenderInfo, 0, sizeof(VenomDebugRenderInfo));
  VenomModuleRender(memory);
  ImGui::Render();
}