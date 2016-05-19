#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
struct DebugMaterialInfo {
  const char *filenames[MaterialTextureType_COUNT];
	V3 diffuse_color;
	V3 specular_color;
};

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
	
  U8 *map_data[MaterialTextureType_COUNT];
	int map_width[MaterialTextureType_COUNT];
	int map_height[MaterialTextureType_COUNT];
	int map_components[MaterialTextureType_COUNT];

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
			map_data[i] = null;
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
			if (map_data[i] == null) continue;

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
			if (map_data[i] == null) continue;
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

void DestroyMaterialData(MaterialData* data) {
  assert(data->textureData != 0);
  assert(data->textureWidth > 0 && data->textureHeight > 0);
  free(data->textureData);
  data->textureWidth = 0;
  data->textureHeight = 0;
  data->flags = 0;
}
