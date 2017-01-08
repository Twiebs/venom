#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb_image.h"

bool CreateMaterialData(const char **filenames, U32 flags, MaterialData *data, RGB8 diffuseTint = {}) {
  
  if(flags == 0) {
    data->materialFlags = MaterialFlag_DIFFUSE;
    data->textureWidth = 1;
    data->textureHeight = 1;
    data->textureData = (uint8_t *)malloc(3);
    data->textureData[0] = diffuseTint.r;
    data->textureData[1] = diffuseTint.g;
    data->textureData[2] = diffuseTint.b;
    return true;
  }

  int materialTextureWidth = 0;
  int materialTextureHeight = 0;
  int materialTextureCount = 0;
  uint8_t *materialTextureData[MaterialTextureType_COUNT] = {};

  for(size_t i = 0; i < MaterialTextureType_COUNT; i++){
    if((flags & (1 << i)) == 0) continue;
    int textureWidth = 0;
    int textureHeight = 0;
    int textureComponents = 0;
    materialTextureData[i] = stbi_load(filenames[i], &textureWidth, &textureHeight, &textureComponents, 3);
    if (materialTextureData[i] == 0) {
      LogError("Failed to read texture: %s", filenames[i]);
      return false;
    }

    if(materialTextureWidth == 0) materialTextureWidth = textureWidth;
    if(materialTextureHeight == 0) materialTextureHeight = textureHeight;
    if(materialTextureWidth != textureWidth || materialTextureHeight != textureHeight) {
      LogWarning("Materials must specificy texture of the same size");
    }
    materialTextureCount++;
    assert(textureComponents == 3);
  }


  data->materialFlags = flags;
  data->textureWidth = materialTextureWidth;
  data->textureHeight = materialTextureHeight;
  size_t sizePerTexture = data->textureWidth * data->textureHeight * 3;
  data->textureData = (uint8_t *)malloc(sizePerTexture * materialTextureCount);
  uint8_t *textureWrite = data->textureData;
  for (size_t i = 0; i < MaterialTextureType_COUNT; i++) {
    if ((flags & (1 << i)) == 0) continue;
    memcpy(textureWrite, materialTextureData[i], sizePerTexture);
    textureWrite += sizePerTexture; 
  }

  for (size_t i = 0; i < MaterialTextureType_COUNT; i++) {
    if ((flags & (1 << i)) == 0) continue;
    free(materialTextureData[i]);
  }

  return true;
}

#if 0
void CreateMaterialDataFromTextureFiles(MaterialData* data, const char* diffuse_filename,
	const char* normal_filename, const char* specular_filename, MemoryBlock *memblock)
{
	int diffuse_width, diffuse_height, diffuse_components;
	int normal_width, normal_height, normal_components;
	int specular_width, specular_height, specular_components;
	
  U8* diffuse_data = stbi_load(diffuse_filename, &diffuse_width,
    &diffuse_height, &diffuse_components, 3);
	U8* normal_data = stbi_load(normal_filename, &normal_width,
    &normal_height, &normal_components, 3);
	U8* specular_data = stbi_load(specular_filename, &specular_width,
    &specular_height, &specular_components, 3);

	assert(diffuse_data != nullptr && normal_data != nullptr && specular_data != nullptr);
	assert(diffuse_width == normal_width && diffuse_width == specular_width);
	assert(diffuse_height == normal_height && diffuse_height == specular_height);
	assert(diffuse_components == 3 && normal_components == 3 && specular_components == 3);

	data->textureWidth = diffuse_width;
	data->textureHeight = diffuse_height;
	size_t texture_memory_size = diffuse_width * diffuse_height * 3;
	data->textureData = PushSize(texture_memory_size * 3, memblock);
	memcpy(data->textureData + (texture_memory_size * 0), diffuse_data, texture_memory_size);
	memcpy(data->textureData + (texture_memory_size * 1), normal_data, texture_memory_size);
	memcpy(data->textureData + (texture_memory_size * 2), specular_data, texture_memory_size);
	stbi_image_free(diffuse_data);
	stbi_image_free(normal_data);
	stbi_image_free(specular_data);
}

void CreateMaterialData(MaterialData* data, 
    DebugMaterialInfo *info, MemoryBlock *memblock) 	{
	
  U8 *map_data[MaterialTextureType_COUNT] = {};
	int map_width[MaterialTextureType_COUNT] = {};
	int map_height[MaterialTextureType_COUNT] = {};
	int map_components[MaterialTextureType_COUNT] = {};

	for (int i = 0; i < MaterialTextureType_COUNT; i++) {
		if (info->filenames[i] != 0 && 
        strcmp(info->filenames[i], VENOM_ASSET_FILE("")) 
        && strcmp(info->filenames[i], "")) {

			map_data[i] = stbi_load(info->filenames[i], &map_width[i],
        &map_height[i], &map_components[i], 0);
			if (map_data[i] == nullptr) {
				LOG_ERROR("Failed to open texture: %s", info->filenames[i]);
			} else {
				data->flags |= 1 << i;
			}
		} else {
			map_data[i] = nullptr;
		}
	}

	bool there_is_a_invalid_texture = false;
	bool there_is_a_valid_texture = false;
	fori((U32)MaterialTextureType_COUNT) {
		if (map_data[i] == NULL)
			there_is_a_invalid_texture = true;
		else
			there_is_a_valid_texture = true;
	}

	//assert(there_is_a_valid_texture != there_is_a_invalid_texture);

	if (there_is_a_valid_texture) {
		size_t required_memory_size = 0;
		fori((U32)MaterialTextureType_COUNT) {
			if (map_components[i] == 4 && i == 0)
				data->flags |= MaterialFlag_TRANSPARENT;
			if (map_data[i] == nullptr) continue;

			assert(map_width[i] == map_width[0]);
			assert(map_height[i] == map_height[0]);
			required_memory_size += (map_width[i] * map_height[i]) * map_components[i];
		}

		data->textureWidth = map_width[0];
		data->textureHeight = map_height[0];
		//data->textureData = PushSize(required_memory_size, memblock);
    data->textureData = (U8*)malloc(required_memory_size);
		uintptr_t write_offset = 0;
		for (int i = 0; i < MaterialTextureType_COUNT; i++) {
			if (map_data[i] == nullptr) continue;
			size_t write_size = map_width[i] * map_height[i] * map_components[i];
			memcpy(data->textureData + write_offset, map_data[i], write_size);
      write_offset += write_size;
			stbi_image_free(map_data[i]);
		}
	}

	else {
		data->textureWidth = 1;
		data->textureHeight = 1;
		//data->textureData = PushSize(3 * 3, memblock);
    data->textureData = (U8*)malloc(3 * 3);
		data->textureData[0] = (U8)(info->diffuse_color.x * 255);
		data->textureData[1] = (U8)(info->diffuse_color.y * 255);
		data->textureData[2] = (U8)(info->diffuse_color.z * 255);
	}
}
#endif

void DestroyMaterialData(MaterialData* data) {
  assert(data->textureData != 0);
  assert(data->textureWidth > 0 && data->textureHeight > 0);
  free(data->textureData);
  data->textureWidth = 0;
  data->textureHeight = 0;
  data->materialFlags = 0;
}

void DestroyModelData(ModelData* data){
  fori(data->meshCount){
    DestroyMaterialData(&data->materialDataPerMesh[i]);
  }

  //This is the pointer returned by allocator
  free(data->index_count_per_mesh);
}
