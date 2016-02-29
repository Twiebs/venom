


#ifndef VENOM_RELEASE
#include "imgui.cpp"
#include "imgui_draw.cpp"

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

	M4 screenProjection = Orthographic(0.0f, system->screen_width, 0.0f, system->screen_height, -1.0f, 100.0f, -1.0f);
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

static inline void imgui_update_state(GameMemory *memory)
{
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

inline void imgui_init(GameMemory* memory)
{
	SystemInfo *system = &memory->systemInfo;
	ImGuiIO& io = ImGui::GetIO();
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
}
#endif


#ifdef VENOM_HOTLOAD
extern "C" void OnModuleLoad(GameMemory *memory)
{
#define EngineAPI(returnType, name, ...) name = memory->engineAPI.name;
EngineAPIList
#undef EngineAPI
#define _(signature, name) name = memory->engineAPI.name;
#include "opengl_procs.h"
#undef _ 
	
	imgui_init(memory);
	int width, height;
	unsigned char *pixels;
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	io.Fonts->TexID = (void *)(size_t)memory->renderState.imguiFontTexture;
}
#endif


