
struct FontVertex {
  V3 position;
  V2 texcoord;
};

struct FontVertexBuffer {
  GLuint vao;
  GLuint vbo;
  size_t maxVertices;
};

void InitFontVertexBuffer(FontVertexBuffer *fontBuffer, size_t maxChars);
void UploadFontVertexData(FontVertexBuffer * fontBuffer, FontVertex *vertices, size_t count);