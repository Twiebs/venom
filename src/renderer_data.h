static const V4 COLOR_WHITE = V4(1.0f, 1.0f, 1.0f, 1.0f);
static const V4 COLOR_BLACK = V4(0.0f, 0.0f, 0.0f, 1.0f);
static const V4 COLOR_GRAY = V4(0.5f, 0.5f, 0.5f, 1.0f);
static const V4 COLOR_RED = V4(1.0f, 0.0f, 0.0f, 1.0f);
static const V4 COLOR_GREEN = V4(0.0f, 1.0f, 0.0f, 1.0f);
static const V4 COLOR_BLUE = V4(0.0f, 0.0f, 1.0f, 1.0f);
static const V4 COLOR_YELLOW = V4(1.0f, 1.0f, 0.0f, 1.0f);
static const V4 COLOR_MAGENTA = V4(1.0f, 0.0f, 1.0f, 1.0f);

struct RGB8 {
  U8 r, g, b;
};

struct RGBA8 {
  U8 r, g, b, a;
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
  S32 joint_index[4];
  F32 weight[4];
};


enum MaterialTextureType {
	MaterialTextureType_DIFFUSE,
	MaterialTextureType_NORMAL,
	MaterialTextureType_SPECULAR,
	MaterialTextureType_COUNT
};

const char *MaterialTextureTypeNames[] = {
	"MaterialTextureType_DIFFUSE",
	"MaterialTextureType_NORMAL",
	"MaterialTextureType_SPECULAR",
	"MaterialTextureType_COUNT"
};

enum MaterialFlag {
	MaterialFlag_DIFFUSE      = 1 << 0,
	MaterialFlag_NORMAL       = 1 << 1,
	MaterialFlag_SPECULAR     = 1 << 2,
	MaterialFlag_TRANSPARENT  = 1 << 3,
	MaterialFlag_REPEAT       = 1 << 4,
};

struct MaterialData {
	U32 materialFlags;
	U16 textureWidth;
	U16 textureHeight;
	U8 *textureData;
};

struct Animation_Joint {
  M4 parent_realtive_matrix;
  M4 inverse_bind_matrix;
  M4 bind_pose_matrix;
  S32 parent_index;
  S32 sibling_index;
  S32 child_index;
  char name[64];
};

struct Joint_Translation_Info {
  F32 time;
  V3 translation;
};

struct Joint_Rotation_Info {
  F32 time;
  Quaternion rotation;
};

struct Joint_Scale_Info {
  F32 time;
  F32 scale;
};

//TODO(Torin) Consider packing scale into translations for alignemtn + packing
//TODO(Torin) Remove the test dynamic arrays
struct Joint_Animation {
  U32 joint_index;
  U32 translation_count;
  U32 rotation_count;
  U32 scaling_count;
  DynamicArray<Joint_Translation_Info> translations;
  DynamicArray<Joint_Rotation_Info> rotations;
  DynamicArray<Joint_Scale_Info> scalings;
};

struct Animation_Clip {
  char name[64];
  F32 duration;
  U32 joint_count;
  DynamicArray<Joint_Animation> joint_animations;
};

struct MeshData {
	U32 vertexCount;
	U32 indexCount;
	AnimatedVertex *vertices;
	U32 *indices;
};

struct ModelData {
	U32 meshCount;
  U32 jointCount;
  U32 animation_clip_count;
  MeshData meshData;
 
  Animation_Joint *joints;
	MaterialData *materialDataPerMesh;
  Animation_Clip *animation_clips;
  U32 *index_count_per_mesh; //This will be the pointer that is returned by allocator
  U32 *joint_count_per_mesh;
};

struct Frustum {
	F32 field_of_view;
	F32 aspect_ratio;
	F32 near_plane_distance;
	F32 far_plane_distance;
	V3 points[8];
};

struct Camera {
	V3 position;
	V3 front;
	F32 fov;
	F32 yaw, pitch;
	F32 near_clip, far_clip;
	//F32 viewportWidth, viewportHeight;
	M4 view, projection;
};

struct DirectionalLight {
	V3 direction;
	V3 color;
};

struct PointLight {
	V3 position;
	V3 color;
	F32 radius;
};