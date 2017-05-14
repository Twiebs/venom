#if 0
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
#endif

enum AssetType {
  AssetType_MATERIAL,
  AssetType_MODEL,
  AssetType_SHADER,
  AssetType_SOUND,
  AssetType_COUNT,
};

enum AssetState {
  AssetState_Unloaded,
  AssetState_Unloading,
  AssetState_Loading,
  AssetState_Loaded,
  AssetState_Invalid,
};

static const char* AssetTypeNames[] = {
  "Material", "Model",
  "Shader", "Sound",
};

static const char *AssetStateNames[] = {
  "Unloaded", "Loading",
  "Loaded", "Invalid"
};

struct Asset_ID {
  U32 reload_counter_value;
  U32 slot_index;
  char asset_name[64];
};

struct AssetSlot {
  AssetState assetState;
  SpinLock lock;

  void* asset;

//NOTE(Torin) For now these are dynamicly allocated
#ifndef VENOM_RELEASE
  
  char *name;
  char *filename;
  U64 lastWriteTime;
#endif//VENOM_RELEASE
};
