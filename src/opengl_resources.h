#pragma once

static const V4 COLOR_BLACK = V4(0.0f, 0.0f, 0.0f, 1.0f);
static const V4 COLOR_GRAY = V4(0.5f, 0.5f, 0.5f, 1.0f);
static const V4 COLOR_RED = V4(1.0f, 0.0f, 0.0f, 1.0f);
static const V4 COLOR_GREEN = V4(0.0f, 1.0f, 0.0f, 1.0f);
static const V4 COLOR_BLUE = V4(0.0f, 0.0f, 1.0f, 1.0f);
static const V4 COLOR_YELLOW = V4(1.0f, 1.0f, 0.0f, 1.0f);
static const V4 COLOR_MAGENTA = V4(1.0f, 0.0f, 1.0f, 1.0f);

struct Vertex2D
{
	V2 position;
	V2 texcoord;
	V4 color;
};

struct Vertex3D
{
	V3 position;
	V3 normal;
	V3 tangent;
	V2 texcoord;
};

struct BoneVertexInfluence
{
	float weight;
	U32 index;
};

struct Bone
{
	M4 offset_matrix;
};

struct AnimatedVertex
{
	V3 position;
	V3 normal;
	V3 tangent;
	V2 texcoord;
	U32 boneInfluenceCount;
	BoneVertexInfluence *bones;
};

struct MeshData
{
	U32 vertexCount;
	U32 indexCount;
	Vertex3D *vertices;
	U32 *indices;
};

struct AnimatedMeshData
{
	U32 vertexCount;
	U32 indexCount;
	U32 boneCount;
	AnimatedVertex *vertices;
	Bone *bones;
	U32 *indices;
};

enum MaterialFlag
{
	MaterialFlag_DIFFUSE = 1 << 0,
	MaterialFlag_NORMAL = 1 << 1,
	MaterialFlag_SPECULAR = 1 << 2,
	MaterialFlag_TRANSPARENT = 1 << 3,
	MaterialFlag_REPEAT = 1 << 4,
};

struct MaterialData
{
	U32 flags;
	U16 textureWidth;
	U16 textureHeight;
	U8 *textureData;
};

struct MaterialDrawable
{
	U32 flags;
	GLuint diffuse_texture_id;
	GLuint normal_texture_id;
	GLuint specular_texture_id;
};

struct ModelData
{
	U32 meshCount;
	MeshData meshData;
	MaterialData *materialDataPerMesh;	
	U32 *indexCountPerMesh;
};

struct AnimatedModelData
{
	U32 meshCount;
	AnimatedMeshData meshData;
	MaterialData *materialDataPerMesh;
	U32 *indexCountPerMesh;
};

struct ModelDrawable
{
	GLuint vertexArrayID;
	U32 meshCount;
	U32 *indexCountPerMesh;
	MaterialDrawable *materials;
};

struct IndexedVertexArray
{
	GLuint vertexArrayID;
	GLuint vertexBufferID;
	GLuint indexBufferID;
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

inline void CreateIndexedVertexArray3D(IndexedVertexArray *vertexArray, U32 vertexCount, U32 indexCount,
	const Vertex3D *vertices, const U32 *indices, GLenum drawMode)
{
	CreateIndexedVertex3DArray(&vertexArray->vertexArrayID, &vertexArray->vertexBufferID, &vertexArray->indexBufferID,
		vertexCount, indexCount, vertices, indices, drawMode);
}

inline void CreateIndexedVertexArray3D(IndexedVertexArray *vertexArray, MeshData *data, GLenum drawMode = GL_STATIC_DRAW)
{
	CreateIndexedVertex3DArray(&vertexArray->vertexArrayID, &vertexArray->vertexBufferID, &vertexArray->indexBufferID,
		data->vertexCount, data->indexCount, data->vertices, data->indices, drawMode);
}

inline void CreateIndexedVertexArray3D(IndexedVertexArray *vertexArray, ModelData *data)
{
	CreateIndexedVertex3DArray(&vertexArray->vertexArrayID, &vertexArray->vertexBufferID, &vertexArray->indexBufferID,
		data->meshData.vertexCount, data->meshData.indexCount, data->meshData.vertices, data->meshData.indices, GL_STATIC_DRAW);
}

inline void CreateIndexedVertex2DArray(IndexedVertexArray *array, 
	U32 vertexCount, U32 indexCount, const Vertex2D *vertices, 
	const U32 *indices, GLenum drawMode)
{
	CreateIndexedVertex2DArray(&array->vertexArrayID, &array->vertexBufferID, &array->indexBufferID,
		vertexCount, indexCount, vertices, indices, drawMode);
}
