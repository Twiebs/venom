#include "renderer_data.cpp"

#ifdef VENOM_OPENGL
#define _(signature, name) static signature name;
#include "opengl_procedures.h"
#undef _

#include "opengl_debug.cpp"
#include "opengl_glsl.cpp"
#include "opengl_resources.cpp"
#include "opengl_render.cpp"
#include "offline_asset_tools.cpp"
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

void PushModelDrawCommand(U32 slotIndex, VenomDrawList* drawList, 
V3 position, V3 rotation = V3{0,0,0})
{
  assert(drawList->modelDrawComandCount + 1 < ARRAY_COUNT(drawList->modelDrawCommands));
  VenomModelDrawCommand& cmd = 
    drawList->modelDrawCommands[drawList->modelDrawComandCount++];
  cmd.modelID = slotIndex;
  cmd.modelMatrix =  Translate(position) * Rotate(rotation);
}

void PushOutlinedModelDrawCommand(U32 modelSlotIndex, VenomDrawList* drawList, 
V3 position, V3 rotation = V3{0,0,0})
{
  VenomModelDrawCommand cmd;
  cmd.modelID = modelSlotIndex;
  cmd.modelMatrix = Translate(position) * Rotate(rotation);
  //TODO(Torin: May 31, 2016)Remove these when done testing
  cmd.position = position;
  cmd.rotation = rotation; 
  StaticArrayAdd(cmd, &drawList->outlinedModelDrawCommandCount, 
   drawList->outlinedModelDrawCommands);
}

