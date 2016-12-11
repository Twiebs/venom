
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
  _(Atmosphere, "Atmosphere.vert", "Atmosphere.frag", "") \
  _(atmospheric_scattering_glsl, "vertexarray_quad.vert", "atmospheric_scattering_glsl.frag", "")

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
  AssetType_MATERIAL,
  AssetType_MODEL,
  AssetType_SHADER,
  AssetType_SOUND,
  AssetType_COUNT,
};

enum AssetFlag {
  AssetFlag_LOADED = 1 << 0,
  AssetFlag_INVALID = 1 << 1,
};

static const char* AssetTypeNames[] = {
  "Material",
  "Model",
  "Shader",
  "Sound",
};

struct Asset_ID {
  U32 reload_counter_value;
  U32 slot_index;
  char asset_name[64];
};

//NOTE(Torin) For now these are dynamicly allocated
struct AssetSlot {
#ifndef VENOM_RELEASE
  char *name;
  char *filename;
  U64 lastWriteTime;
#endif//VENOM_RELEASE
  U64 flags;
  void* asset;
};

struct MaterialAsset {
  U64 flags;
  const char* filenames[MaterialTextureType_COUNT];
  MaterialData data;
  MaterialDrawable drawable;
};

//TODO(Torin) Seriously consider just conolodating all of this into one
//structure... There really is not much reason to keep all of this stuff seperatly
struct ModelAsset {
  U32 slot_index;
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

struct Shader_Asset {
  GLuint program_id;
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

struct AssetManifest {
#ifndef VENOM_RELEASE
  union { 
    struct {
      DynamicArray<AssetSlot> materialAssets;
      DynamicArray<AssetSlot> modelAssets;
      DynamicArray<AssetSlot> shaderAssets;
      DynamicArray<AssetSlot> soundAssets;
    };
    DynamicArray<AssetSlot> assetSlotArrays[AssetType_COUNT];
  };

  U32 modelReloadCounter;

#else//VENOM_REALEASE
  AssetSlot material_assets[Material_Asset_COUNT];
  AssetSlot model_assets[Model_Asset_COUNT];
  AssetSlot shader_assets[Shader_Asset_COUNT];
  AssetSlot sound_asset[Sound_Asset_COUNT];
#endif//VENOM_RELEASE


	DEBUGLoadedShader loadedShaders[DEBUGShaderID_COUNT];
  MaterialAssetList materialAssetList;

  //TODO(Torin) This is bad beacuse we might end up deleteing assets
  //at runtime so some dynamic strings is probably the best approach
#if 0
  char *stringBlock;
  U64 stringBlockSize;
  U64 stringBlockEntryCount;

	MemoryBlock memoryBlock; //TODO(Torin)Currently not being used!
#endif
};


static DEBUGShaderInfo DEBUG_SHADER_INFOS[] =
{
#define _(name,v,f,g) {{ VENOM_SHADER_FILE(v), VENOM_SHADER_FILE(f), VENOM_SHADER_FILE(g) }},
	DebugShaderInfoList
#undef _ 
};

ModelAsset *GetModelAsset(Asset_ID& id, AssetManifest* assets);
ModelDrawable* GetModelDrawable(Asset_ID& id, AssetManifest* manifest);
ModelDrawable* GetModelDrawableFromIndex(U32 modelIndex, AssetManifest *manifest);

MaterialAsset *GetMaterialAsset(U32 index, AssetManifest *assets);

GLuint GetShaderProgram(DEBUGShaderID shaderID, AssetManifest *assets);



const MaterialDrawable& GetMaterial(U32 id, AssetManifest* assets);

#ifndef VENOM_RELEASE
void WriteAssetManifestFile(const char *filename, AssetManifest *manifest);
void ReadAssetManifestFile(const char *filename, AssetManifest *manifest);
static void HotloadShaders(AssetManifest *assets);


//static void HotloadModels(AssetManifest* manifest);

Asset_ID GetModelID(const char *name, AssetManifest* assets);


U32 GetShaderID(const char *name, AssetManifest* assets);

void DestroyModelAsset(U32 index, AssetManifest *manifest);
#endif//!VENOM_RELEASE