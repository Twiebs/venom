
void InitFontVertexBuffer(FontVertexBuffer *fontBuffer, size_t maxChars) {
  glGenVertexArrays(1, &fontBuffer->vao);
  glGenBuffers(1, &fontBuffer->vbo);
  glBindVertexArray(fontBuffer->vao);
  glBindBuffer(GL_ARRAY_BUFFER, fontBuffer->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(FontVertex) * 6 * maxChars, 0, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FontVertex), (void *)offsetof(FontVertex, position));
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(FontVertex), (void *)offsetof(FontVertex, texcoord));
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  fontBuffer->maxVertices = maxChars * 6;
}

void UploadFontVertexData(FontVertexBuffer *fontBuffer, FontVertex *vertices, size_t count) {
  assert(count < fontBuffer->maxVertices);
  glBindBuffer(GL_ARRAY_BUFFER, fontBuffer->vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(FontVertex), vertices);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}