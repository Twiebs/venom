
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

struct MaterialAsset {
  U64 flags;
  const char* filenames[MaterialTextureType_COUNT];
  MaterialData data;
  MaterialDrawable drawable;
};

//TODO(Torin) Seriously consider just conolodating all of this into one
//structure... There really is not much reason to keep all of this stuff seperatly
//yet until we know exatly how the rest of the engine will process the data.  Then
//the structure can be seperated for cache locality...  It would not work like this
//in the first place anyway!
struct ModelAsset {
  U32 slotIndex; //TODO(Torin) Can we get rid of this?
  V3 size;
  AABB aabb; //TODO(Torin) This doesnt need to be stored just keep the size
  

  //ModelData data;

  U32 meshCount;
  U32 jointCount;
  U32 animationClipCount;
  U32 vertexCount;
  U32 indexCount;
  
  AnimatedVertex *vertices;
  U32 *indices;
  Animation_Joint *joints;
  Animation_Clip *animationClips;

  U32 *indexCountPerMesh;
  MaterialData *materialDataPerMesh;
  IndexedVertexArray vertexArray;

#if 0
  struct ModelDrawable {
    GLuint vertexArrayID;
    U32 meshCount;
    
    U32 joint_count;
    MaterialDrawable *materials;
    Animation_Joint *joints;
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
  };
#endif

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

#include <atomic>

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

  std::atomic<U32> modelReloadCounter;

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

ModelAsset *GetModelAsset(Asset_ID& id);
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