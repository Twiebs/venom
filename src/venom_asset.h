
#define DebugShaderInfoList					\
	_(Debug, "debug_shader.vert", "debug_shader.frag", "")\
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
  _(DeferredMaterial, "vertexarray_quad.vert", "material_deferred.frag", "")

#define DebugModelList       \
	_(rock00, "dumb_rock.dae") \
	_(bamboo, "foliage/bamboo/model.fbx") \
	_(bush,   "foliage/bush/model.fbx")   \
	_(white_flower, "foliage/white_flower/model.fbx") \
	_(player, "dumb_player.fbx") \
  _(SmallBarrel, "containers/SmallBarrel/barrel.fbx")\
  _(Lamp, "lamp.fbx") \
  _(Stairs, "Stairs/Stairs.fbx")

constexpr U64 StringHashDJB2(const char* str) {
  U64 hash = 5381;
  while(*str) {
    hash = ((hash << 5) + hash) + *str;
    str++;
  }
  return hash;
} 


struct MaterialAsset {
  U32 flags;
  MaterialData data;
  MaterialDrawable drawable;
  const char* filenames[MaterialTextureType_COUNT];
};

struct MaterialAssetList {
  MaterialAsset* materials;
  U64 materialCount;
};

struct AssetDataCache {
	MemoryBlock memory;
};

struct DEBUGShaderInfo {
	const char *filenames[4];
};


struct DEBUGModelInfo {
	const char *filename;
};

struct DEBUGLoadedShader {
	const char *filenames[4];
	U64 lastWriteTimes[4];
	GLuint programHandle;
	bool is_loaded;
};

struct SoundAssetSlot
{
	SoundData data;
};

struct DEBUGLoadedModel {
	ModelData data;
  AABB aabb;
	IndexedVertexArray vertexArray;
	ModelDrawable drawable;
};

static DEBUGShaderInfo DEBUG_SHADER_INFOS[] =
{
#define _(name,v,f,g) {{ VENOM_SHADER_FILE(v), VENOM_SHADER_FILE(f), VENOM_SHADER_FILE(g) }},
	DebugShaderInfoList
#undef _ 
};

static DEBUGModelInfo DEBUG_MODEL_INFOS[] =
{
#define _(name, file) { VENOM_ASSET_FILE(file) },
	DebugModelList
#undef _ 
};



enum DEBUGModelID
{
#define _(name, file) DEBUGModelID_##name, 
	DebugModelList
#undef _ 
	DEBUGModelID_COUNT
};

enum DEBUGShaderID
{
#define _(name,v,f,g)ShaderID_##name,
	DebugShaderInfoList
#undef _ 
	DEBUGShaderID_COUNT
};

struct GameAssets
{
	MemoryBlock memory;

	DEBUGLoadedShader loadedShaders[DEBUGShaderID_COUNT];
	DEBUGLoadedModel loadedModels[DEBUGModelID_COUNT];
  MaterialAssetList materialAssetList;
};

