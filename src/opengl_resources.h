
//TODO(Torin 2016-12-05) Remove RenderGroups

#if 1
struct RenderGroup {
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
	MemoryBlock vertexBlock;
	MemoryBlock indexBlock;
	size_t current_vertex_count;
	size_t current_index_count;
};
#endif

struct IndexedVertexArray {
  GLuint vertexArrayID;
  GLuint vertexBufferID;
  GLuint indexBufferID;
  U32 vertexCount, indexCount;
};

struct CascadedShadowMap {
  GLuint framebuffer;
  GLuint depthTexture;
  Frustum cascadeFrustums[SHADOW_MAP_CASCADE_COUNT];
  M4 lightSpaceTransforms[SHADOW_MAP_CASCADE_COUNT];
  M4 lightProjectionMatrices[SHADOW_MAP_CASCADE_COUNT];
	F32 shadowCascadeDistances[SHADOW_MAP_CASCADE_COUNT];
};

struct OmnidirectionalShadowMap {
  GLuint depthCubemap;
};

struct GBuffer {
  GLuint framebuffer;
  GLuint positionDepth;
  GLuint normal;
  GLuint albedoSpecular;
  GLuint depthStencilBuffer;
};

struct SSAO {
  GLuint framebuffer;
  GLuint bufferTexture;
  GLuint noiseTexture;
  V3 kernelSamples[SSAO_SAMPLE_COUNT];
};

struct MaterialDrawable {
	U32 flags;
	GLuint diffuse_texture_id;
	GLuint normal_texture_id;
	GLuint specular_texture_id;
};

//TODO(Torin) This thing is a massive
//pain in the ass... remove it... mabye...
//If they are stored in a different location
//from the model asset it could prove to be usefull...

struct ModelDrawable {
	GLuint vertexArrayID;
	U32 meshCount;
	U32 *indexCountPerMesh;
  U32 *jointCountPerMesh;
  U32 joint_count;
	MaterialDrawable *materials;
  Animation_Joint *joints;
}; //40 bytes



static const U32 OMNI_SHADOW_MAP_RESOLUTION = 4096;

void InitGBuffer(GBuffer* gbuffer, const U32 viewportWidth, const U32 viewportHeight);
void InitCascadedShadowMaps(CascadedShadowMap* csm, 
  F32 viewportWidth, F32 viewportHeight, F32 fieldOfView);
void InitOmnidirectionalShadowMap(OmnidirectionalShadowMap* osm);
void SSAOInit(SSAO* ssao, U32 viewportWidth, U32 viewportHeight);

//TODO(Torin) Remove the return value here
MaterialDrawable CreateMaterialDrawable(MaterialData *data);
void DestroyMaterialDrawable(MaterialDrawable* drawable);


void CreateIndexedVertex3DArray(GLuint *vertexArray, 
  GLuint *vertexBuffer, GLuint *indexBuffer, U32 vertexCount, U32 indexCount, 
  const Vertex3D *vertices, const U32 *indices, GLenum drawMode);

void CreateIndexedVertex2DArray(GLuint *vertexArray, GLuint *vertexBuffer, 
  GLuint *indexBuffer, U32 vertexCount, U32 indexCount, 
  const Vertex2D *vertices, const U32 *indices, GLenum drawMode); 

void create_indexed_animated_vertex_array(GLuint *vertexArray, GLuint *vertexBuffer, GLuint *indexBuffer,
    U32 vertexCount, U32 indexCount, const AnimatedVertex *vertices, const U32 *indices, GLenum drawMode);

void DeleteIndexedVertexArray(GLuint *vao, GLuint *vbo, GLuint *ebo);

GLuint CreateTextureWithMipmaps(const U8* pixels, U16 width, U16 height, 
	GLenum internal_format, GLenum format, GLenum wrapMode = GL_REPEAT, GLenum filterMode = GL_LINEAR);
GLuint CreateTextureWithoutMipmaps(const char *filename, GLenum wrapMode = GL_REPEAT, GLenum filterMode = GL_LINEAR);

void CreateMaterialDataFromTextureFiles(MaterialData* data, const char* diffuse_filename,
	const char* normal_filename, const char* specular_filename, MemoryBlock *memblock);

//=====================================================================================================

inline void CreateIndexedVertexArray3D(IndexedVertexArray *vertexArray, const Vertex3D *vertices, 
const U32 *indices, const U32 vertexCount, const U32 indexCount, const GLenum drawMode) {
	CreateIndexedVertex3DArray(
    &vertexArray->vertexArrayID, 
    &vertexArray->vertexBufferID, 
    &vertexArray->indexBufferID,
		vertexCount, indexCount, 
    vertices, indices, drawMode);
  vertexArray->vertexCount = vertexCount;
  vertexArray->indexCount = indexCount;
}

inline void CreateIndexedVertexArray3D(IndexedVertexArray *vertexArray, 
const U32 vertexCount, const U32 indexCount, const GLenum drawMode) {
	CreateIndexedVertex3DArray(
    &vertexArray->vertexArrayID, 
    &vertexArray->vertexBufferID, 
    &vertexArray->indexBufferID,
		vertexCount, indexCount, 
    0, 0, drawMode);
  vertexArray->vertexCount = vertexCount;
  vertexArray->indexCount = indexCount;
}

inline void CreateIndexedVertexArray3D(IndexedVertexArray *vertexArray, MeshData *data, GLenum drawMode = GL_STATIC_DRAW) {
	CreateIndexedVertex3DArray(&vertexArray->vertexArrayID, &vertexArray->vertexBufferID, &vertexArray->indexBufferID,
		data->vertexCount, data->indexCount, (Vertex3D *)data->vertices, data->indices, drawMode);
}

inline void CreateIndexedVertex2DArray(IndexedVertexArray *array, 
	U32 vertexCount, U32 indexCount, const Vertex2D *vertices, 
	const U32 *indices, GLenum drawMode) {
	CreateIndexedVertex2DArray(&array->vertexArrayID, &array->vertexBufferID, &array->indexBufferID,
		vertexCount, indexCount, vertices, indices, drawMode);
}

inline void create_indexed_animated_vertex_array(IndexedVertexArray *vertex_array, MeshData *data) {
  create_indexed_animated_vertex_array(&vertex_array->vertexArrayID, &vertex_array->vertexBufferID, &vertex_array->indexBufferID,
    data->vertexCount, data->indexCount, data->vertices, data->indices, GL_STATIC_DRAW);
}
