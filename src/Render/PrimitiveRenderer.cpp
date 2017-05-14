
//@HACK @HACK @HACK!!!!
static FontVertexBuffer g_fontBuffer;

DebugDrawCommand* PrimitiveRenderer::AddDrawCommand(DebugDrawCommandType type, V4 color, B8 isSolid, F32 duration) {
  DebugDrawCommand* cmd = drawCommands.AddElement();
  cmd->type = type;
  cmd->isSolid = isSolid;
  cmd->color = color;
  cmd->duration = duration;
  cmd->isLightingEnabled = isLightingEnabled;
  return cmd;
}

void PrimitiveRenderer::DirectionalLight(V3 direction) {
  lightDirection = Normalize(direction);
}

void PrimitiveRenderer::Box(V3 min, V3 max, V4 color, bool isSolid, F32 duration) {
  auto cmd = AddDrawCommand(DebugDrawCommand_BOX, color, isSolid, duration);
  cmd->min = min;
  cmd->max = max;
}

void PrimitiveRenderer::Box(const AABB& a, V4 color, bool isSolid, F32 duration) {
  auto cmd = AddDrawCommand(DebugDrawCommand_BOX, color, isSolid, duration);
  cmd->min = a.min;
  cmd->max = a.max;
}

void PrimitiveRenderer::Sphere(V3 center, F32 radius, V4 color, bool isSolid, F32 duration) {
  auto cmd = AddDrawCommand(DebugDrawCommand_SPHERE, color, isSolid, duration);
  cmd->center = center;
  cmd->radius = radius;
}

void PrimitiveRenderer::Line(V3 start, V3 end, V4 color, bool isSolid, F32 duration) {
  auto cmd = AddDrawCommand(DebugDrawCommand_LINE, color, isSolid, duration);
  cmd->lineSegmentPositions[0] = start;
  cmd->lineSegmentPositions[1] = end;
}

void PrimitiveRenderer::Triangle(V3 a, V3 b, V3 c, V4 color, B8 isSolid, F32 duration) {
  auto cmd = AddDrawCommand(DebugDrawCommand_TRIANGLE, color, isSolid, 0.0f);
  cmd->a = a;
  cmd->b = b;
  cmd->c = c;
}

void PrimitiveRenderer::Text(const char *text, V3 position, V4 color, F32 duration) {
  auto cmd = textCommands.AddElement();
  cmd->color = color;
  cmd->duration = duration;
  cmd->position = position;
  cmd->text = text;
}

void PrimitiveRenderer::Axis(V3 position, F32 duration) {
  auto cmd = drawCommands.AddElement();
  cmd->type = DebugDrawCommand_AXIS;
  cmd->position = position;
  cmd->isSolid = true;
  cmd->duration = duration;
}

void PrimitiveRenderer::Render(const Camera *camera, const F32 deltaTime) {
  static const U32 MODEL_MATRIX_LOCATION = 0;
  static const U32 VIEW_MATRIX_LOCATION = 1;
  static const U32 PROJECTION_MATRIX_LOCATION = 2;
  static const U32 COLOR_LOCATION = 3;
  static const U32 LINE_SEGMENT_POSITIONS_LOCATION = 4;
  static const U32 USE_LINE_POSITION_LOCATION = 7;

  static const U32 IS_LIGHTING_ENABLED_LOCATION = 8;
  static const U32 LIGHT_DIRECTION_LOCATION = 9;
  static const U32 NORMAL_LOCATION = 10;

  //TODO(Torin) @HACK THIS IS A HUGEEEE @HACK FIX THIS
  //TODO(Torin) @HACK THIS IS A HUGEEEE @HACK FIX THIS
  //TODO(Torin) @HACK THIS IS A HUGEEEE @HACK FIX THIS
  DebugRenderer *globaDebugRenderer = &g_debug_renderer;
  M4 modelMatrix = M4Identity();
  auto assetManifest = GetAssetManifest();
  glUseProgram(GetShaderProgram(ShaderID_DebugShape, assetManifest));
  SetUniform(MODEL_MATRIX_LOCATION, modelMatrix);
  SetUniform(VIEW_MATRIX_LOCATION, camera->viewMatrix);
  SetUniform(PROJECTION_MATRIX_LOCATION, camera->projectionMatrix);
  glBindVertexArray(globaDebugRenderer->vao);

  for (size_t i = 0; i < drawCommands.count; i++) {
    DebugDrawCommand* cmd = &drawCommands[i];

    if (cmd->isSolid == false) {
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
      glEnable(GL_CULL_FACE);
      glEnable(GL_DEPTH_TEST);
    }

    SetUniform(COLOR_LOCATION, cmd->color);
    SetUniform(IS_LIGHTING_ENABLED_LOCATION, cmd->isLightingEnabled);
    SetUniform(LIGHT_DIRECTION_LOCATION, lightDirection);

    switch (cmd->type) {

    case DebugDrawCommand_BOX: {
      V3 boundsSize = Abs(cmd->max - cmd->min);
      modelMatrix = Translate(cmd->min + (boundsSize * 0.5f)) * Scale(boundsSize);
      glUniformMatrix4fv(MODEL_MATRIX_LOCATION, 1, GL_FALSE, &modelMatrix[0][0]);
      glDrawElements(GL_TRIANGLES, globaDebugRenderer->cubeIndexCount, GL_UNSIGNED_INT, (void *)(uintptr_t)(globaDebugRenderer->cubeIndexOffset));
    } break;

    case DebugDrawCommand_SPHERE: {
      modelMatrix = Translate(cmd->center) * Scale(cmd->radius);
      glUniformMatrix4fv(MODEL_MATRIX_LOCATION, 1, GL_FALSE, &modelMatrix[0][0]);
      glDrawElements(GL_TRIANGLES, globaDebugRenderer->sphereIndexCount,
        GL_UNSIGNED_INT, (void*)(uintptr_t)globaDebugRenderer->sphereIndexOffset);
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
      V3 ab = cmd->b - cmd->a;
      V3 ac = cmd->c - cmd->a;
      V3 normal = Normalize(Cross(ab, ac));
      SetUniform(NORMAL_LOCATION, normal);
      SetUniform(USE_LINE_POSITION_LOCATION, 1);
      modelMatrix = M4Identity();
      SetUniform(MODEL_MATRIX_LOCATION, modelMatrix);
      SetUniform(LINE_SEGMENT_POSITIONS_LOCATION + 0, cmd->c);
      SetUniform(LINE_SEGMENT_POSITIONS_LOCATION + 1, cmd->b);
      SetUniform(LINE_SEGMENT_POSITIONS_LOCATION + 2, cmd->a);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      SetUniform(USE_LINE_POSITION_LOCATION, 0);
    } break;

    case DebugDrawCommand_AXIS: {
      M4 translation = Translate(cmd->position);
      glUniformMatrix4fv(MODEL_MATRIX_LOCATION, 1, GL_FALSE, &translation[0][0]);
      glUniform4f(COLOR_LOCATION, 1.0, 0.0, 0.0, 1.0);
      glDrawElements(GL_TRIANGLES, globaDebugRenderer->axisIndexCount,
        GL_UNSIGNED_INT, (void*)(uintptr_t)globaDebugRenderer->axisIndexOffset);
      glUniform4f(COLOR_LOCATION, 0.0, 1.0, 0.0, 1.0);
      M4 model = translation * Rotate(0.0, 0.0, -DEG2RAD*90.0f);
      glUniformMatrix4fv(MODEL_MATRIX_LOCATION, 1, GL_FALSE, &model[0][0]);
      glDrawElements(GL_TRIANGLES, globaDebugRenderer->axisIndexCount,
        GL_UNSIGNED_INT, (void*)(uintptr_t)globaDebugRenderer->axisIndexOffset);
      glUniform4f(COLOR_LOCATION, 0.0, 0.0, 1.0, 1.0);
      model = translation * Rotate(0.0, -DEG2RAD*90.0f, 0.0f);
      glUniformMatrix4fv(MODEL_MATRIX_LOCATION, 1, GL_FALSE, &model[0][0]);
      glDrawElements(GL_TRIANGLES, globaDebugRenderer->axisIndexCount,
        GL_UNSIGNED_INT, (void*)(uintptr_t)globaDebugRenderer->axisIndexOffset);
    } break;

    case DebugDrawCommand_CAMERA: {
      M4 translation = Translate(cmd->position);
      SetUniform(MODEL_MATRIX_LOCATION, translation);
      SetUniform(COLOR_LOCATION, cmd->color);
      glDrawElements(GL_TRIANGLES, globaDebugRenderer->sphereIndexCount,
        GL_UNSIGNED_INT, (void*)(uintptr_t)globaDebugRenderer->sphereIndexOffset);
    } break;

    default: {
      assert(false);
    } break;

    }

    cmd->duration -= deltaTime;
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  { //Render text commands
    auto memory = GetVenomEngineData();
    glUseProgram(GetShaderProgram(ShaderID_Font, GetAssetManifest()));
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, memory->renderState.imguiFontTexture);
    M4 inverseViewMatrix = Inverse(camera->viewMatrix);
    for (size_t i = 0; i < textCommands.count; i++) {
      auto& textCommand = textCommands[i];
      size_t textLength = strlen(textCommand.text);
      U32 offset = Memory::FrameStackPush(textLength * sizeof(FontVertex) * 6);
      FontVertex *vertices = (FontVertex *)Memory::FrameStackPointer(offset);

      //Generate the font vertices
      auto font = ImGui::GetFont();
      F32 scale = 1.0f / 24.0f;
      float totalWidth = 0.0f;
      float totalHeight = 0.0f;
      for (size_t i = 0; i < textLength; i++) {
        auto glyph = font->FindGlyph(textCommand.text[i]);
        F32 glyphWidth = glyph->X1 - glyph->X0;
        F32 glyphHeight = glyph->Y1 - glyph->Y0;
        totalWidth += glyph->XAdvance;
        totalHeight = Max(totalHeight, glyphHeight);
      }
      totalWidth *= scale;
      totalHeight *= scale;

#if 1
      V3 cameraRight = V3(inverseViewMatrix * V4(1.0, 0.0f, 0.0, 0.0));
      V3 cameraUp = V3(inverseViewMatrix * V4(0.0, 1.0, 0.0, 0.0));
#else
      V3 cameraRight = V3(1.0, 0.0, 0.0);
      V3 cameraUp = V3(0.0, 1.0, 0.0);
#endif

      V3 glyphOrigin = textCommand.position - (cameraRight*(totalWidth*0.5f)) - (cameraUp * (totalHeight * 0.5f));
      //V3 glyphOrigin = textCommand.position;
      for (size_t i = 0; i < textLength; i++) {
        auto glyph = font->FindGlyph(textCommand.text[i]);
        F32 glyphWidth = (glyph->X1 - glyph->X0) * scale;
        F32 glyphHeight = (glyph->Y1 - glyph->Y0) * scale;

        vertices[i * 6 + 0] = FontVertex { glyphOrigin, V2(glyph->U0, glyph->V1) };
        vertices[i * 6 + 1] = FontVertex { glyphOrigin + (cameraRight * glyphWidth), V2(glyph->U1, glyph->V1) };
        vertices[i * 6 + 2] = FontVertex { glyphOrigin + (cameraRight * glyphWidth) - (cameraUp * glyphHeight), V2(glyph->U1, glyph->V0) };
        
        vertices[i * 6 + 3] = FontVertex{ glyphOrigin, V2(glyph->U0, glyph->V1) };
        vertices[i * 6 + 4] = FontVertex{ glyphOrigin + (cameraRight * glyphWidth) - (cameraUp * glyphHeight), V2(glyph->U1, glyph->V0) };
        vertices[i * 6 + 5] = FontVertex{ glyphOrigin - (cameraUp * glyphHeight), V2(glyph->U0, glyph->V0) };
        glyphOrigin += cameraRight * (glyph->XAdvance * scale);
      }


      UploadFontVertexData(&g_fontBuffer, vertices, textLength * 6);
      static const S32 MVP_MATRIX_LOCATION = 0;
      static const S32 COLOR_LOCATION = 1;
      M4 mvp = camera->projectionMatrix * camera->viewMatrix;
      SetUniform(MVP_MATRIX_LOCATION, mvp);
      SetUniform(COLOR_LOCATION, textCommand.color);
      glBindVertexArray(g_fontBuffer.vao);
      glBindBuffer(GL_ARRAY_BUFFER, g_fontBuffer.vbo);
      glDrawArrays(GL_TRIANGLES, 0, textLength * 6);
    }
    glDisable(GL_BLEND);
  }

  size_t newDebugCommandCount = 0;
  for (size_t i = 0; i < drawCommands.count; i++) {
    if (drawCommands[i].duration > 0) {
      drawCommands[newDebugCommandCount] = drawCommands[i];
      newDebugCommandCount++;
    }
  }

  drawCommands.count = newDebugCommandCount;
  textCommands.count = 0;
}

