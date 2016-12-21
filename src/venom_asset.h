
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

struct AssetSlot {
//NOTE(Torin) For now these are dynamicly allocated
#ifndef VENOM_RELEASE
  char *name;
  char *filename;
  U64 lastWriteTime;
#endif//VENOM_RELEASE
  U64 flags;
  void* asset;
};
