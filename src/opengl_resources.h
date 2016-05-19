struct RenderGroup {
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
	MemoryBlock vertexBlock;
	MemoryBlock indexBlock;
	size_t current_vertex_count;
	size_t current_index_count;
};

struct MaterialDrawable {
	U32 flags;
	GLuint diffuse_texture_id;
	GLuint normal_texture_id;
	GLuint specular_texture_id;
};

struct ModelDrawable {
	GLuint vertexArrayID;
	U32 meshCount;
	U32 *indexCountPerMesh;
	MaterialDrawable *materials;
};

struct IndexedVertexArray {
	GLuint vertexArrayID;
	GLuint vertexBufferID;
	GLuint indexBufferID;
  U32 vertexCount, indexCount;
};

void CreateIndexedVertex3DArray(GLuint *vertexArray, GLuint *vertexBuffer, GLuint *indexBuffer,
	U32 vertexCount, U32 indexCount, const Vertex3D *vertices, const U32 *indices, GLenum drawMode);
void CreateIndexedVertex2DArray(GLuint *vertexArray, GLuint *vertexBuffer, GLuint *indexBuffer,
	U32 vertexCount, U32 indexCount, const Vertex2D *vertices, const U32 *indices, GLenum drawMode);
void DeleteIndexedVertexArray(GLuint *vao, GLuint *vbo, GLuint *ebo);

GLuint CreateTextureWithMipmaps(const U8* pixels, U16 width, U16 height, 
	GLenum internal_format, GLenum format, GLenum wrapMode = GL_REPEAT, GLenum filterMode = GL_LINEAR);
GLuint CreateTextureWithoutMipmaps(const char *filename, GLenum wrapMode = GL_REPEAT, GLenum filterMode = GL_LINEAR);

void CreateMaterialDataFromTextureFiles(MaterialData* data, const char* diffuse_filename,
	const char* normal_filename, const char* specular_filename, MemoryBlock *memblock);

inline void CreateIndexedVertexArray3D(IndexedVertexArray *vertexArray, 
    const Vertex3D *vertices, const U32 *indices, 
    const U32 vertexCount, const U32 indexCount, 
    const GLenum drawMode)
{
	CreateIndexedVertex3DArray(
      &vertexArray->vertexArrayID, 
      &vertexArray->vertexBufferID, 
      &vertexArray->indexBufferID,
		  vertexCount, indexCount, 
      vertices, indices, drawMode);
  vertexArray->vertexCount = vertexCount;
  vertexArray->indexCount = indexCount;
}

inline void CreateIndexedVertexArray3D(IndexedVertexArray *vertexArray, MeshData *data, GLenum drawMode = GL_STATIC_DRAW)
{
	CreateIndexedVertex3DArray(&vertexArray->vertexArrayID, &vertexArray->vertexBufferID, &vertexArray->indexBufferID,
		data->vertexCount, data->indexCount, data->vertices, data->indices, drawMode);
}

inline void CreateIndexedVertexArray3D(IndexedVertexArray *vertexArray, ModelData *data)
{
	CreateIndexedVertex3DArray(
      &vertexArray->vertexArrayID, 
      &vertexArray->vertexBufferID, 
      &vertexArray->indexBufferID, 
      data->meshData.vertexCount, 
      data->meshData.indexCount, 
      data->meshData.vertices, 
      data->meshData.indices, 
      GL_STATIC_DRAW);
}

inline void CreateIndexedVertex2DArray(IndexedVertexArray *array, 
	U32 vertexCount, U32 indexCount, const Vertex2D *vertices, 
	const U32 *indices, GLenum drawMode)
{
	CreateIndexedVertex2DArray(&array->vertexArrayID, &array->vertexBufferID, &array->indexBufferID,
		vertexCount, indexCount, vertices, indices, drawMode);
}
