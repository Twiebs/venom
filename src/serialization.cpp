#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#include <fstream>

template<typename TArchive>
void serialize(AssetManifest& manifest, TArchive& archive) {
  for (size_t i = 0; i < manifest.modelAssets.count; i++) {
    AssetSlot *slot = &manifest.modelAssets[i];
  }
}

static inline void serialize_asset_manifest(AssetManifest *manifest) {
  std::ofstream out_stream = std::ofstream("test.json");
  cereal::JSONOutputArchive archive(out_stream);
}