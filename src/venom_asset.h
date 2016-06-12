
#define DebugShaderInfoList					\
	_(Debug, "debug_shader.vert", "debug_shader.frag", "")\
	_(DebugShape, "DebugShape.vert", "DebugShape.frag", "")\
	_(SingleColor, "debug_shader.vert", "single_color.frag", "") \
	_(Sprite, "basic_sprite.vert", "basic_sprite.frag", "") \
	_(terrain, "terrain_instanced.vert", "terrain.frag", "") \
	_(debug_normals, "debug_normals.vert", "debug_normals.frag", "debug_normals.geom") \
	_(material_opaque, "material_opaque.vert", "material_opaque.frag", "") \
	_(material_transparent, "material_transparent.vert", "material_transparent.frag", "") \
	_(skydome, "skydome.vert", "skydome.frag", "") \
	_(depth_map, "depth_map.vert", "depth_map.frag", "") \
	_(debug_depth_map, "vertexarray_quad.vert", "debug_depth_map.frag", "")\
  _(depth_cubemap, "depth_cubemap.vert", "depth_cubemap.frag", "depth_cubemap.geom") \
  _(GBuffer, "gbuffer.vert", "gbuffer.frag", "") \
  _(SSAO, "vertexarray_quad.vert", "ssao.frag", "") \
  _(DeferredMaterial, "vertexarray_quad.vert", "material_deferred.frag", "") \
  _(Atmosphere, "Atmosphere.vert", "Atmosphere.frag", "")

#define DebugModelList       \
  _(rock00, "dumb_rock.dae") \
  _(bamboo, "foliage/bamboo/model.fbx") \
  _(bush,   "foliage/bush/model.fbx")   \
  _(white_flower, "foliage/white_flower/model.fbx") \
  _(player, "dumb_player.fbx") \
  _(SmallBarrel, "containers/SmallBarrel/barrel.blend")\
  _(Lamp, "lamp.fbx") \
  _(Stairs, "Stairs/Stairs.fbx") \
  _(StanfordDragon, "dragon.obj") \
  _(UtahTeapot, "UtahTeapot.fbx") \
  _(HappyBuddah, "buddah.obj") \
  _(Mitsuba, "mitsuba.obj") \
  _(Bookshelf, "bookshelf.fbx") \
  _(Table, "table.blend") \

#ifdef VENOM_RELEASE
#define ModelID(name) ModelID_##name
#define MaterialID(name) MaterialID_##name
#define SoundID(name) MusicID_##name
#define ShaderID(name) ShaderID_##name
#else//!VENOM_RELEASE
#define ModelID(name) #name
#define MaterialID(name) #name
#define SoundID(name) #name
#define ShaderID(name) #name
#endif//VENOM_RELEASE

enum AssetType {
  AssetType_Material,
  AssetType_Model,
  AssetType_Shader,
  AssetType_Sound,
  AssetType_Count,
};

enum AssetFlag {
  AssetFlag_Loaded = 1 << 0,
};

static const char* AssetTypeNames[] = {
  "Material",
  "Model",
  "Shader",
  "Sound",
};


struct AssetSlot {
  U64 flags;
  //NOTE(Torin) For now these are dynamicly allocated
  char *name;
  char *filename;
  void* asset;
};

struct MaterialAsset{
  U64 flags;
  const char* filenames[MaterialTextureType_COUNT];
  MaterialData data;
  MaterialDrawable drawable;
};

struct ModelAsset{
#ifndef VENOM_RELEASE
  const char *name;
  U64 lastWriteTime;
#endif//VENOM_RELEASE

  ModelData data;
  AABB aabb;
  V3 size;
  IndexedVertexArray vertexArray;
  ModelDrawable drawable;
};

struct DEBUGLoadedShader {
	const char *filenames[4];
	U64 lastWriteTimes[4];
	GLuint programHandle;
	bool is_loaded;
};

struct SoundAsset {
	SoundData data;
};

struct MaterialAssetList {
  MaterialAsset* materials;
  U64 materialCount;
};

struct DEBUGShaderInfo {
	const char *filenames[4];
};
enum DEBUGShaderID {
#define _(name,v,f,g)ShaderID_##name,
	DebugShaderInfoList
#undef _ 
	DEBUGShaderID_COUNT
};


//NOTE(Torin) In non-release builds The asset struct contains dynamic arrays 
//so that the number of assets can grow during runtime for quick iterration times
//During release mode the assets are stored as static arrays
struct AssetManifest {
  DynamicArray<AssetSlot> modelAssets;


	DEBUGLoadedShader loadedShaders[DEBUGShaderID_COUNT];
  MaterialAssetList materialAssetList;

  //TODO(Torin) This is bad beacuse we might end up deleteing assets
  //at runtime so some dynamic strings is probably the best approach
  char *stringBlock;
  U64 stringBlockSize;
  U64 stringBlockEntryCount;

	MemoryBlock memoryBlock; //TODO(Torin)Currently not being used!
};


static DEBUGShaderInfo DEBUG_SHADER_INFOS[] =
{
#define _(name,v,f,g) {{ VENOM_SHADER_FILE(v), VENOM_SHADER_FILE(f), VENOM_SHADER_FILE(g) }},
	DebugShaderInfoList
#undef _ 
};

ModelAsset *GetModelAsset(U32 index, AssetManifest* assets);
MaterialAsset *GetMaterialAsset(U32 index, AssetManifest *assets);

GLuint GetShaderProgram(DEBUGShaderID shaderID, AssetManifest *assets);
const ModelDrawable& GetModelDrawable(U32 slotIndex, AssetManifest* manifest);
const MaterialDrawable& GetMaterial(U32 id, AssetManifest* assets);

#ifndef VENOM_RELEASE
void WriteAssetManifestFile(const char *filename, AssetManifest *manifest);
void ReadAssetManifestFile(const char *filename, AssetManifest *manifest);
static void HotloadShaders(AssetManifest *assets);
static void HotloadModels(AssetManifest* manifest);
S64 GetModelID(const char *name, AssetManifest* assets);
S64 GetShaderID(const char *name, AssetManifest* assets);

void DestroyModelAsset(U32 index, AssetManifest *manifest);
#endif//!VENOM_RELEASE

//const MaterialDrawable& GetMaterialDrawable(U32 id, AssetManifest* manifest);
//const GLuint GetShaderProgram(DEBUGShaderID shaderID, AssetManifest* manifest);
//const ModelDrawable& GetModelDrawable(DEBUGModelID id, GameAssets* assets);





