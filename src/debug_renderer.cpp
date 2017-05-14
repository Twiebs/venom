
static DebugRenderer g_debug_renderer;
void initalize_debug_renderer() {
  DebugRenderer *debugRenderer = &g_debug_renderer;
  //NOTE(Torin) Contains definitions for debug primitave vertices and indices
  #include "render_debug_shapes.h"

  glGenVertexArrays(1, &debugRenderer->vao);
  glBindVertexArray(debugRenderer->vao);

#define ShapeList \
  _(cube) _(sphere) _(axis) 

  glGenBuffers(1, &debugRenderer->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, debugRenderer->vbo);
#define _(name) + sizeof(name##Vertices)
  glBufferData(GL_ARRAY_BUFFER, 0 ShapeList, 0, GL_DYNAMIC_DRAW);
#undef _


  {
    size_t currentOffset = 0;
#define _(name) glBufferSubData(GL_ARRAY_BUFFER, currentOffset, \
    sizeof(name##Vertices), name##Vertices); \
    currentOffset += sizeof(name##Vertices);
    ShapeList
#undef _
  }



  glGenBuffers(1, &debugRenderer->ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, debugRenderer->ebo);
#define _(name) + sizeof(name##Indices)
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0 ShapeList, 0, GL_DYNAMIC_DRAW);
#undef _

  static auto fuck = [](U32 *indices, size_t count, U32 offset) {
    for (size_t i = 0; i < count; i++) {
      indices[i] += offset;
    }
  };

  {
    size_t currentVertexCount = 0;
    size_t currentIndicesOffset = 0;
#define _(name) fuck(name##Indices, ARRAY_COUNT(name##Indices), currentVertexCount); \
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, currentIndicesOffset, sizeof(name##Indices), name##Indices); \
  currentVertexCount += ARRAY_COUNT(name##Vertices);\
  currentIndicesOffset += sizeof(name##Indices);
    ShapeList
#undef _
  }

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glBindVertexArray(0);

  {
    size_t currentOffset = 0;
#define _(name) debugRenderer->name##IndexCount = ARRAY_COUNT(name##Indices); \
    debugRenderer->name##IndexOffset = currentOffset; \
    currentOffset += sizeof(name##Indices);
    ShapeList
#undef _
  }
}


static inline DebugDrawCommand* add_debug_draw_command(DebugDrawCommandType type, V4 color, bool isSolid, F32 duration) {
  DebugRenderer *debugRenderer = &g_debug_renderer;
  DebugDrawCommand* cmd = debugRenderer->drawCommands.AddElement();
  cmd->type = type;
  cmd->isSolid = isSolid;
  cmd->color = color;
  cmd->duration = duration;
  return cmd;
}

void draw_debug_box(V3 min, V3 max, V4 color, bool isSolid, F32 duration) {
  auto cmd = add_debug_draw_command(DebugDrawCommand_BOX, color, isSolid, duration);
  cmd->min = min;
  cmd->max = max;
}

void draw_debug_sphere(V3 center, F32 radius, V4 color, bool isSolid, F32 duration) {
  auto cmd = add_debug_draw_command(DebugDrawCommand_SPHERE, color, isSolid, duration);
  cmd->center = center;
  cmd->radius = radius;
}

void draw_debug_line(V3 start, V3 end, V4 color, bool isSolid, F32 duration) {
  auto cmd = add_debug_draw_command(DebugDrawCommand_LINE, color, isSolid, duration);
  cmd->lineSegmentPositions[0] = start;
  cmd->lineSegmentPositions[1] = end;
}

void draw_debug_axes(V3 position) {
  DebugRenderer *debugRenderer = &g_debug_renderer;
  DebugDrawCommand *cmd = debugRenderer->drawCommands.AddElement();
  cmd->type = DebugDrawCommand_AXIS;
  cmd->position = position;
  cmd->isSolid = true;
}

void draw_debug_plane(V3 a, V3 b, V3 c, V4 color, bool isSolid, F32 duration) {


}

void draw_debug_camera(Camera *camera) {
  DebugDrawCommand *cmd = add_debug_draw_command(DebugDrawCommand_CAMERA, V4(1.0f, 1.0f, 0.0f, 1.0f), true, 0.0f);
  cmd->position = camera->position;
}

void render_debug_draw_commands(Camera *camera, AssetManifest *assetManifest, F32 deltaTime) {
  static const U32 MODEL_MATRIX_LOCATION = 0;
  static const U32 VIEW_MATRIX_LOCATION = 1;
  static const U32 PROJECTION_MATRIX_LOCATION = 2;
  static const U32 COLOR_LOCATION = 3;
  static const U32 LINE_SEGMENT_POSITIONS_LOCATION = 4;
  static const U32 USE_LINE_POSITION_LOCATION = 6;

  DebugRenderer *debugRenderer = &g_debug_renderer;
  M4 modelMatrix = M4Identity();
  glUseProgram(GetShaderProgram(ShaderID_DebugShape, assetManifest));
  SetUniform(MODEL_MATRIX_LOCATION, modelMatrix);
  SetUniform(VIEW_MATRIX_LOCATION, camera->viewMatrix);
  SetUniform(PROJECTION_MATRIX_LOCATION, camera->projectionMatrix);
  glBindVertexArray(debugRenderer->vao);

  for (size_t i = 0; i < debugRenderer->drawCommands.count; i++) {
    DebugDrawCommand* cmd = &debugRenderer->drawCommands[i];
    if (cmd->isSolid == false) {
      glDisable(GL_CULL_FACE);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glUniform4fv(COLOR_LOCATION, 1, &cmd->color.x);
    switch (cmd->type) {

    case DebugDrawCommand_BOX: {
      V3 boundsSize = Abs(cmd->max - cmd->min);
      modelMatrix = Translate(cmd->min + (boundsSize * 0.5f)) * Scale(boundsSize);
      glUniformMatrix4fv(MODEL_MATRIX_LOCATION, 1, GL_FALSE, &modelMatrix[0][0]);
      glDrawElements(GL_TRIANGLES, debugRenderer->cubeIndexCount, GL_UNSIGNED_INT, (void *)(uintptr_t)(debugRenderer->cubeIndexOffset));
    } break;

    case DebugDrawCommand_SPHERE: {
      modelMatrix = Translate(cmd->center) * Scale(cmd->radius);
      glUniformMatrix4fv(MODEL_MATRIX_LOCATION, 1, GL_FALSE, &modelMatrix[0][0]);
      glDrawElements(GL_TRIANGLES, debugRenderer->sphereIndexCount,
        GL_UNSIGNED_INT, (void*)(uintptr_t)debugRenderer->sphereIndexOffset);
    } break;

    case DebugDrawCommand_LINE: {
      glUniform1i(USE_LINE_POSITION_LOCATION, 1);
      modelMatrix = M4Identity();
      glUniformMatrix4fv(MODEL_MATRIX_LOCATION, 1, GL_FALSE, &modelMatrix[0][0]);
      SetUniform(LINE_SEGMENT_POSITIONS_LOCATION + 0, cmd->lineSegmentPositions[0]);
      SetUniform(LINE_SEGMENT_POSITIONS_LOCATION + 1, cmd->lineSegmentPositions[1]);
      glDisable(GL_DEPTH_TEST);
      glDrawArrays(GL_LINES, 0, 2);
      glEnable(GL_DEPTH_TEST);
      glUniform1i(USE_LINE_POSITION_LOCATION, 0);
    } break;

    case DebugDrawCommand_TRIANGLE: {
      modelMatrix = M4Identity();
      SetUniform(MODEL_MATRIX_LOCATION, modelMatrix);

    } break;

    case DebugDrawCommand_AXIS: {
      M4 translation = Translate(cmd->position);
      glUniformMatrix4fv(MODEL_MATRIX_LOCATION, 1, GL_FALSE, &translation[0][0]);
      glUniform4f(COLOR_LOCATION, 1.0, 0.0, 0.0, 1.0);
      glDrawElements(GL_TRIANGLES, debugRenderer->axisIndexCount,
        GL_UNSIGNED_INT, (void*)(uintptr_t)debugRenderer->axisIndexOffset);
      glUniform4f(COLOR_LOCATION, 0.0, 1.0, 0.0, 1.0);
      M4 model = translation * Rotate(0.0, 0.0, -DEG2RAD*90.0f);
      glUniformMatrix4fv(MODEL_MATRIX_LOCATION, 1, GL_FALSE, &model[0][0]);
      glDrawElements(GL_TRIANGLES, debugRenderer->axisIndexCount,
        GL_UNSIGNED_INT, (void*)(uintptr_t)debugRenderer->axisIndexOffset);
      glUniform4f(COLOR_LOCATION, 0.0, 0.0, 1.0, 1.0);
      model = translation * Rotate(0.0, DEG2RAD*90.0f, 0.0f);
      glUniformMatrix4fv(MODEL_MATRIX_LOCATION, 1, GL_FALSE, &model[0][0]);
      glDrawElements(GL_TRIANGLES, debugRenderer->axisIndexCount,
        GL_UNSIGNED_INT, (void*)(uintptr_t)debugRenderer->axisIndexOffset);
    } break;

    case DebugDrawCommand_CAMERA: {
      M4 translation = Translate(cmd->position);
      SetUniform(MODEL_MATRIX_LOCATION, translation);
      SetUniform(COLOR_LOCATION, cmd->color);
      glDrawElements(GL_TRIANGLES, debugRenderer->sphereIndexCount,
        GL_UNSIGNED_INT, (void*)(uintptr_t)debugRenderer->sphereIndexOffset);
    } break;

    default: {
      assert(false);
    } break;

    }

    cmd->duration -= deltaTime;
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_CULL_FACE);
  }

  size_t newDebugCommandCount = 0;
  for (size_t i = 0; i < debugRenderer->drawCommands.count; i++) {
    if (debugRenderer->drawCommands[i].duration > 0) {
      debugRenderer->drawCommands[newDebugCommandCount] = debugRenderer->drawCommands[i];
      newDebugCommandCount++;
    }
  }
  debugRenderer->drawCommands.count = newDebugCommandCount;
}