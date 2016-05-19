static const V4 COLOR_BLACK = V4(0.0f, 0.0f, 0.0f, 1.0f);
static const V4 COLOR_GRAY = V4(0.5f, 0.5f, 0.5f, 1.0f);
static const V4 COLOR_RED = V4(1.0f, 0.0f, 0.0f, 1.0f);
static const V4 COLOR_GREEN = V4(0.0f, 1.0f, 0.0f, 1.0f);
static const V4 COLOR_BLUE = V4(0.0f, 0.0f, 1.0f, 1.0f);
static const V4 COLOR_YELLOW = V4(1.0f, 1.0f, 0.0f, 1.0f);
static const V4 COLOR_MAGENTA = V4(1.0f, 0.0f, 1.0f, 1.0f);

struct BoneVertexInfluence {
	float weight;
	U32 index;
};

struct Bone {
	M4 offset_matrix;
};

struct Vertex2D {
	V2 position;
	V2 texcoord;
	V4 color;
};

struct Vertex3D {
	V3 position;
	V3 normal;
	V3 tangent;
	V2 texcoord;
};

struct AnimatedVertex {
	V3 position;
	V3 normal;
	V3 tangent;
	V2 texcoord;
	U32 boneInfluenceCount;
	BoneVertexInfluence *bones;
};

enum MaterialFlag {
	MaterialFlag_DIFFUSE      = 1 << 0,
	MaterialFlag_NORMAL       = 1 << 1,
	MaterialFlag_SPECULAR     = 1 << 2,
	MaterialFlag_TRANSPARENT  = 1 << 3,
	MaterialFlag_REPEAT       = 1 << 4,
};

enum MaterialTextureType {
	MaterialTextureType_DIFFUSE,
	MaterialTextureType_NORMAL,
	MaterialTextureType_SPECULAR,
	MaterialTextureType_COUNT
};

struct MaterialData {
	U32 flags;
	U16 textureWidth;
	U16 textureHeight;
	U8 *textureData;
};

struct MeshData {
	U32 vertexCount;
	U32 indexCount;
	Vertex3D *vertices;
	U32 *indices;
};

struct AnimatedMeshData {
	U32 vertexCount;
	U32 indexCount;
	U32 boneCount;
	AnimatedVertex *vertices;
	Bone *bones;
	U32 *indices;
};

struct ModelData {
	U32 meshCount;
	MeshData meshData;
	MaterialData *materialDataPerMesh;	
	U32 *indexCountPerMesh;
};

struct AnimatedModelData {
	U32 meshCount;
	AnimatedMeshData meshData;
	MaterialData *materialDataPerMesh;
	U32 *indexCountPerMesh;
};

struct DebugMaterialInfo {
	const char *filenames[MaterialTextureType_COUNT];
	V3 diffuse_color;
	V3 specular_color;
};