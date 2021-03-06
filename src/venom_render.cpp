#include "renderer_data.cpp"
#include "animation.cpp"

#ifdef VENOM_OPENGL
#define _(signature, name) static signature name;
#include "opengl_procedures.h"
#undef _

#include "opengl_debug.cpp"
#include "opengl_glsl.cpp"
#include "opengl_resources.cpp"
#include "opengl_render.cpp"
#include "assets/offline_asset_tools.cpp"
#endif//VENOM_OPENGL

#define StaticArrayAdd(element, countptr, array) \
  assert((*countptr + 1) < ARRAY_COUNT(array)); \
  array[*countptr] = element; \
  *countptr = *countptr + 1;

void PushDrawCommand(IndexedVertexArray* vertexArray, VenomDrawList* drawList) {
  assert(drawList->drawCommandCount + 1 < ARRAY_COUNT(drawList->drawCommands));
  VenomDrawCommand* drawCommand = &drawList->drawCommands[drawList->drawCommandCount++];
  drawCommand->vertexArrayID = vertexArray->vertexArrayID;
  drawCommand->indexCount = vertexArray->indexCount;
}

void PushMeshDrawCommand(U32 vertexArrayID, U32 materialID, 
U32 indexCount, U32 indexOffset, VenomDrawList* drawList){
  assert(drawList->meshDrawCommandCount + 1 < ARRAY_COUNT(drawList->meshDrawCommands));
  VenomMeshDrawCommand& cmd = drawList->meshDrawCommands[drawList->meshDrawCommandCount++];
  cmd.vertexArrayID = vertexArrayID;
  cmd.materialID = materialID;
  cmd.indexCount = indexCount;
  cmd.indexOffset = indexOffset;
}

void AddStaticModelToDrawList(VenomDrawList *drawList, ModelAsset *model, V3 position, V3 rotation = V3(0.0f)) {
  assert(drawList->modelDrawComandCount + 1 < ARRAY_COUNT(drawList->modelDrawCommands));
  if (model == nullptr) return;
  VenomModelDrawCommand& cmd = drawList->modelDrawCommands[drawList->modelDrawComandCount++];
  cmd.model = model;
  cmd.modelMatrix = Translate(position) * Rotate(rotation);
}

void AddOutlinedModelToDrawList(VenomDrawList *drawList, ModelAsset *model, V3 position, V3 rotation = V3(0.0f)) {
  VenomModelDrawCommand cmd;
  cmd.model = model;
  cmd.modelMatrix = Translate(position) * Rotate(rotation);
  //TODO(Torin: May 31, 2016)Remove these when done testing
  cmd.position = position;
  cmd.rotation = rotation;
  StaticArrayAdd(cmd, &drawList->outlinedModelDrawCommandCount,
    drawList->outlinedModelDrawCommands);
}

void AddAnimatedModelToDrawList(VenomDrawList *draw_list, ModelAsset *model, AnimationState *animation_state, V3 position, V3 rotation = V3(0.0, 0.0, 0.0), V3 scale = V3(1.0, 1.0, 1.0)) {  
  Animated_Model_Draw_Command *draw_command = draw_list->animated_model_draw_commands.AddElement();
  draw_command->model_matrix = Translate(position) * Rotate(rotation) * Scale(scale);
  draw_command->animation_state = animation_state;
  draw_command->model = model;
}

