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

static inline
DebugRenderCommand* AddDebugRenderCommand(VenomDrawList* drawList, 
    DebugRenderCommandType type, V4 color, bool isSolid, F32 duration)
{
  assert(drawList->debugCommandCount + 1 < ARRAY_COUNT(drawList->debugCommands));
  DebugRenderCommand* cmd = &drawList->debugCommands[drawList->debugCommandCount];
  cmd->type = type;
  cmd->isSolid = isSolid;
  cmd->color = color;
  cmd->duration = duration;
  drawList->debugCommandCount++;
  return cmd;
}

void AddWireframeBox(VenomDrawList* drawList, 
    V3 min, V3 max, V4 color = COLOR_WHITE, bool isSolid = false, F32 duration = 0.0f) 
{
  auto cmd = AddDebugRenderCommand(drawList, DebugRenderCommandType_Box,
    color, isSolid, duration);
  cmd->min = min;
  cmd->max = max;
}

void AddSphere(VenomDrawList* drawList, V3 center, F32 radius, 
    V4 color = COLOR_WHITE, bool isSolid = false, F32 duration = 0.0f) 
{
  auto cmd = AddDebugRenderCommand(drawList, DebugRenderCommandType_Sphere,
    color, isSolid, duration);
  cmd->center = center;
  cmd->radius = radius;
}

void AddLine(VenomDrawList* drawList, V3 start, V3 end,
  V4 color = COLOR_WHITE, bool isSolid = false, F32 duration = 0.0f)
{
  auto cmd = AddDebugRenderCommand(drawList, DebugRenderCommandType_Line,
    color, isSolid, duration);
  cmd->lineSegmentPositions[0] = start;
  cmd->lineSegmentPositions[1] = end;
}

void AddAxes(V3 position, VenomDrawList *drawList){
  assert(drawList->debugCommandCount + 1 < ARRAY_COUNT(drawList->debugCommands));
  DebugRenderCommand* cmd = &drawList->debugCommands[drawList->debugCommandCount];
  cmd->type = DebugRenderCommandType_Axis;
  cmd->position = position;
  drawList->debugCommandCount++;
}
