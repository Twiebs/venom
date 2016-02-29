#pragma clang diagnostic push 
#pragma clang diagnostic ignored "-Wall"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/scene.h>
#pragma clang diagnostic pop

#pragma clang diagnostic push 
#pragma clang diagnostic ignored "-Wall"
#include "stb_vorbis.c"
#pragma clang diagnostic pop

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

enum DebugMaterialMapType
{
	DebugMaterialMapType_DIFFUSE,
	DebugMaterialMapType_NORMAL,
	DebugMaterialMapType_SPECULAR,
	DebugMaterialMapType_COUNT,
};

struct DebugMaterialInfo
{
	union
	{
		struct
		{
			const char *diffuse_filename;
			const char *normal_filename;
			const char *specular_filename;
		};

		const char *filenames[DebugMaterialMapType_COUNT];
	};

	V3 diffuse_color;
	V3 specular_color;
};

void CreateMaterialDataFromTextureFiles(MaterialData* data, const char* diffuse_filename,
	const char* normal_filename, const char* specular_filename, MemoryBlock *memblock)
{
	int diffuse_width, diffuse_height, diffuse_components;
	int normal_width, normal_height, normal_components;
	int specular_width, specular_height, specular_components;
	uint8_t* diffuse_data = stbi_load(diffuse_filename, &diffuse_width, &diffuse_height, &diffuse_components, 3);
	uint8_t* normal_data = stbi_load(normal_filename, &normal_width, &normal_height, &normal_components, 3);
	uint8_t* specular_data = stbi_load(specular_filename, &specular_width, &specular_height, &specular_components, 3);
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

void CreateMaterialData(MaterialData* data, DebugMaterialInfo *info, MemoryBlock *memblock) 	
{
	U8 *map_data[DebugMaterialMapType_COUNT];
	int map_width[DebugMaterialMapType_COUNT];
	int map_height[DebugMaterialMapType_COUNT];
	int map_components[DebugMaterialMapType_COUNT];
	for (int i = 0; i < DebugMaterialMapType_COUNT; i++) 
	{
		if (strcmp(info->filenames[i], VENOM_ASSET_FILE("")) && strcmp(info->filenames[i], "")) {
			map_data[i] = stbi_load(info->filenames[i], &map_width[i], &map_height[i], &map_components[i], 0);
			if (map_data[i] == null) {
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
	fori((U32)DebugMaterialMapType_COUNT) {
		if (map_data[i] == NULL)
			there_is_a_invalid_texture = true;
		else
			there_is_a_valid_texture = true;
	}

	//assert(there_is_a_valid_texture != there_is_a_invalid_texture);

	if (there_is_a_valid_texture)
	{
		size_t required_memory_size = 0;
		fori((U32)DebugMaterialMapType_COUNT) {
			if (map_components[i] == 4 && i == 0)
				data->flags |= MaterialFlag_TRANSPARENT;
			if (map_data[i] == null) continue;

			assert(map_width[i] == map_width[0]);
			assert(map_height[i] == map_height[0]);
			required_memory_size += (map_width[i] * map_height[i]) * map_components[i];
		}

		data->textureWidth = map_width[0];
		data->textureHeight = map_height[0];
		data->textureData = PushSize(required_memory_size, memblock);
		uintptr_t write_offset = 0;
		for (int i = 0; i < DebugMaterialMapType_COUNT; i++)
		{
			if (map_data[i] == null) continue;
			size_t write_size = map_width[i] * map_height[i] * map_components[i];
			memcpy(data->textureData + write_offset, map_data[i], write_size);
			stbi_image_free(map_data[i]);
		}
	}
	else
	{
		data->textureWidth = 1;
		data->textureHeight = 1;
		data->textureData = PushSize(3 * 3, memblock);
		data->textureData[0] = (U8)(info->diffuse_color.x * 255);
		data->textureData[1] = (U8)(info->diffuse_color.y * 255);
		data->textureData[2] = (U8)(info->diffuse_color.z * 255);
	}
}

SoundData LoadOGG(const char *filename)
{
	short *output = 0;
	int channels, sample_rate;
	stb_vorbis_decode_filename(filename, &channels, &sample_rate, &output);

	int error;
	stb_vorbis *oggfile = stb_vorbis_open_filename(filename, &error, NULL);
	if (!oggfile) LOG_ERROR("FAILED to load ogg file");
	U32 sampleCount = stb_vorbis_stream_length_in_samples(oggfile);
	stb_vorbis_close(oggfile);

	assert(output != nullptr);
	assert(channels == 2);
	assert(sample_rate == 48000);

	SoundData result = {};
	result.sampleCount = sampleCount;
	result.samples = output;
	return result;
}


ModelData ImportExternalModelData(const char *filename, MemoryBlock *memblock)
{
	Assimp::Importer importer;
	auto scene = importer.ReadFile(filename, aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_FlipUVs);
	if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		LOG_ERROR("Assimp failed to load file (%s): %s", filename, importer.GetErrorString());
		return ModelData{};
	}


	ModelData data = {};
	data.meshCount = scene->mNumMaterials;
	for (U32 i = 0; i < scene->mNumMeshes; i++)
	{
		data.meshData.vertexCount += scene->mMeshes[i]->mNumVertices;
		data.meshData.indexCount += scene->mMeshes[i]->mNumFaces * 3;
	}

	//TODO(Torin) This will be broken with a better asset system
	//The required memory needs to be a single comound value made of the following 
	//memory requirements that will be used to query the asset system for a properly sized
	//quantiy of memory
	assert(data.meshCount == scene->mNumMaterials);
	data.indexCountPerMesh = ReserveArray(U32, data.meshCount, memblock);
	data.materialDataPerMesh = ReserveArray(MaterialData, data.meshCount, memblock);
	data.meshData.vertices = ReserveArray(Vertex3D, data.meshData.vertexCount, memblock);
	data.meshData.indices = ReserveArray(U32, data.meshData.indexCount, memblock);

	U32 currentVertexOffset = 0, currentIndexOffset = 0;
	for (U32 i = 0; i < scene->mNumMaterials; i++)
	{
		for (U32 n = 0; n < scene->mNumMeshes; n++)
		{
			if (scene->mMeshes[n]->mMaterialIndex == i)
			{
				aiMesh *assimpMesh = scene->mMeshes[n];
				for (U32 j = 0; j < assimpMesh->mNumVertices; j++) {
					auto index = currentVertexOffset + j;
					data.meshData.vertices[index].position.x = assimpMesh->mVertices[j].x;
					data.meshData.vertices[index].position.y = assimpMesh->mVertices[j].z;
					data.meshData.vertices[index].position.z = assimpMesh->mVertices[j].y;
				}

				for (U32 j = 0; j < assimpMesh->mNumVertices; j++) {
					auto index = currentVertexOffset + j;
					data.meshData.vertices[index].normal.x = assimpMesh->mNormals[j].x;
					data.meshData.vertices[index].normal.y = assimpMesh->mNormals[j].z;
					data.meshData.vertices[index].normal.z = assimpMesh->mNormals[j].y;
				}

				if (assimpMesh->mTangents != nullptr) {
					for (U32 j = 0; j < assimpMesh->mNumVertices; j++) {
						auto index = currentVertexOffset + j;
						data.meshData.vertices[index].tangent.x = assimpMesh->mTangents[j].x;
						data.meshData.vertices[index].tangent.y = assimpMesh->mTangents[j].y;
						data.meshData.vertices[index].tangent.z = assimpMesh->mTangents[j].z;
					}
				}

				if (assimpMesh->mTextureCoords[0] != nullptr) {
					for (U32 j = 0; j < assimpMesh->mNumVertices; j++) {
						auto index = currentVertexOffset + j;
						data.meshData.vertices[index].texcoord.x = assimpMesh->mTextureCoords[0][j].x;
						data.meshData.vertices[index].texcoord.y = assimpMesh->mTextureCoords[0][j].y;
					}
				}

				U32 lastIndexOffset = currentIndexOffset;
				for (U32 j = 0; j < assimpMesh->mNumFaces; j++)
				{
					data.meshData.indices[currentIndexOffset + 0] = assimpMesh->mFaces[j].mIndices[0] + currentVertexOffset;
					data.meshData.indices[currentIndexOffset + 1] = assimpMesh->mFaces[j].mIndices[1] + currentVertexOffset;
					data.meshData.indices[currentIndexOffset + 2] = assimpMesh->mFaces[j].mIndices[2] + currentVertexOffset;
					currentIndexOffset += 3;
				}
				assert(currentIndexOffset == lastIndexOffset + (assimpMesh->mNumFaces * 3));
				currentVertexOffset += scene->mMeshes[n]->mNumVertices;
				data.indexCountPerMesh[i] += scene->mMeshes[n]->mNumFaces * 3;
			}
		}
	}

	auto model_filename = std::string(filename);
	auto lastSlashInFilepath = model_filename.find_last_of("/");
	//auto lastDotInFilepath = model_filename.find_last_of('.');
	auto directory = model_filename.substr(0, lastSlashInFilepath + 1);
	auto GetTextureFilename = [&directory](aiMaterial* material, aiTextureType type) -> std::string { auto textureCount = material->GetTextureCount(type); assert(textureCount <= 1 && "Two maps of the same kind are not currently suported!");
	if (textureCount == 0) return "";

	aiString ai_textureFilename;
	material->GetTexture(type, 0, &ai_textureFilename);
	std::string textureFilename(ai_textureFilename.C_Str());
	auto lastSlash = textureFilename.find_last_of("/");
	if (lastSlash != std::string::npos) {
		textureFilename = textureFilename.substr(lastSlash + 1, textureFilename.size() - lastSlash);
	}

	return textureFilename;
	};

	for (U32 i = 0; i < scene->mNumMaterials; i++) {
		aiMaterial* material = scene->mMaterials[i];
		auto diffuse_filename = GetTextureFilename(material, aiTextureType_DIFFUSE);
		auto normal_filename = GetTextureFilename(material, aiTextureType_NORMALS);
		auto specular_filename = GetTextureFilename(material, aiTextureType_SPECULAR);

		aiColor3D diffuse_color, specular_color;
		material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse_color);
		material->Get(AI_MATKEY_COLOR_SPECULAR, specular_color);

		diffuse_filename = directory + diffuse_filename;
		if (normal_filename != "") 
			normal_filename = directory + normal_filename;
		if (specular_filename != "") 
			specular_filename = directory + specular_filename;

		DebugMaterialInfo info = {};
		info.diffuse_filename = diffuse_filename.c_str();
		info.specular_filename = specular_filename.c_str();
		info.normal_filename = normal_filename.c_str();
		info.diffuse_color = V3(diffuse_color.r, diffuse_color.g, diffuse_color.b);
		info.specular_color = V3(specular_color.r, specular_color.g, specular_color.b);
		CreateMaterialData(&data.materialDataPerMesh[i], &info, memblock);
	}
	return data;
}

#if 0
AnimatedModelData ImportExternalAnimatedModelData(const char *filename, MemoryBlock *memblock)
{
	Assimp::Importer importer;
	auto scene = importer.ReadFile(filename, aiProcess_FlipWindingOrder | aiProcess_Triangulate);
	if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		LOG_ERROR("Assimp failed to load file (%s): %s", filename, importer.GetErrorString());
		return AnimatedModelData{};
	}


	AnimatedModelData data = {};
	data.meshCount = scene->mNumMaterials;
	for (auto i = 0; i < scene->mNumMeshes; i++)
	{
		data.meshData.vertexCount += scene->mMeshes[i]->mNumVertices;
		data.meshData.indexCount += scene->mMeshes[i]->mNumFaces * 3;
	}

	//TODO(Torin) This will be broken with a better asset system
	//The required memory needs to be a single comound value made of the following 
	//memory requirements that will be used to query the asset system for a properly sized
	//quantiy of memory
	assert(data.meshCount == scene->mNumMaterials);
	data.indexCountPerMesh = ReserveArray(U32, data.meshCount, memblock);
	data.materialDataPerMesh = ReserveArray(MaterialData, data.meshCount, memblock);
	//data.meshData.vertices = ReserveArray(AnimatedVertex, data.meshData.vertexCount, memblock);	
	AquireArrayMemory(data.meshData.vertices, data.meshData.vertexCount, memblock);

	data.meshData.indices = ReserveArray(U32, data.meshData.indexCount, memblock);
	
	U32 currentVertexOffset = 0, currentIndexOffset = 0;
	for (U32 i = 0; i < scene->mNumMaterials; i++)
	{
		for (U32 n = 0; n < scene->mNumMeshes; n++)
		{
			if (scene->mMeshes[n]->mMaterialIndex == i)
			{
				aiMesh *assimpMesh = scene->mMeshes[n];
				for (U32 j = 0; j < assimpMesh->mNumVertices; j++) {
					auto index = currentVertexOffset + j;
					data.meshData.vertices[index].position.x = assimpMesh->mVertices[j].x;
					data.meshData.vertices[index].position.y = assimpMesh->mVertices[j].z;
					data.meshData.vertices[index].position.z = assimpMesh->mVertices[j].y;
				}

				for (U32 j = 0; j < assimpMesh->mNumVertices; j++) {
					auto index = currentVertexOffset + j;
					data.meshData.vertices[index].normal.x = assimpMesh->mNormals[j].x;
					data.meshData.vertices[index].normal.y = assimpMesh->mNormals[j].z;
					data.meshData.vertices[index].normal.z = assimpMesh->mNormals[j].y;
				}

				if (assimpMesh->mTangents != nullptr) {
					for (auto j = 0; j < assimpMesh->mNumVertices; j++) {
						auto index = currentVertexOffset + j;
						data.meshData.vertices[index].tangent.x = assimpMesh->mTangents[j].x;
						data.meshData.vertices[index].tangent.y = assimpMesh->mTangents[j].y;
						data.meshData.vertices[index].tangent.z = assimpMesh->mTangents[j].z;
					}
				}

				if (assimpMesh->mTextureCoords[0] != nullptr) {
					for (auto j = 0; j < assimpMesh->mNumVertices; j++) {
						auto index = currentVertexOffset + j;
						data.meshData.vertices[index].texcoord.x = assimpMesh->mTextureCoords[0][j].x;
						data.meshData.vertices[index].texcoord.y = assimpMesh->mTextureCoords[0][j].y;
					}
				}

				U32 lastIndexOffset = currentIndexOffset;
				for (U32 j = 0; j < assimpMesh->mNumFaces; j++)
				{
					data.meshData.indices[currentIndexOffset + 0] = assimpMesh->mFaces[j].mIndices[0] + currentVertexOffset;
					data.meshData.indices[currentIndexOffset + 1] = assimpMesh->mFaces[j].mIndices[1] + currentVertexOffset;
					data.meshData.indices[currentIndexOffset + 2] = assimpMesh->mFaces[j].mIndices[2] + currentVertexOffset;
					currentIndexOffset += 3;
				}



#if 0
				iter(assimpMesh->mNumBones)
				{
					aiBone *bone = assimpMesh->mBones[i];
					data.meshData.	
					
					bone->mOffsetMatrix 
					for (int n = 0; n < bone->mNumWeights; n++)
					{
						aiVertexWeight *weight = bone->mWeights[n];
						weight->

					}
				}
#endif


				assert(currentIndexOffset == lastIndexOffset + (assimpMesh->mNumFaces * 3));
				currentVertexOffset += scene->mMeshes[n]->mNumVertices;
				data.indexCountPerMesh[i] = scene->mMeshes[n]->mNumFaces * 3;
			}
		}
	}

	auto model_filename = std::string(filename);
	auto lastSlashInFilepath = model_filename.find_last_of("/");
	auto lastDotInFilepath = model_filename.find_last_of('.');
	auto directory = model_filename.substr(0, lastSlashInFilepath + 1);
	auto GetTextureFilename = [&directory](aiMaterial* material, aiTextureType type) -> std::string { auto textureCount = material->GetTextureCount(type); assert(textureCount <= 1 && "Two maps of the same kind are not currently suported!");
		if (textureCount == 0) return "";

		aiString ai_textureFilename;
		material->GetTexture(type, 0, &ai_textureFilename);
		std::string textureFilename(ai_textureFilename.C_Str());
		auto lastSlash = textureFilename.find_last_of("/");
		if (lastSlash != std::string::npos) {
			textureFilename = textureFilename.substr(lastSlash + 1, textureFilename.size() - lastSlash);
		}

		return textureFilename;
	};
	
	for (auto i = 0; i < scene->mNumMaterials; i++) {
		aiMaterial* material = scene->mMaterials[i];
		auto diffuse_filename = GetTextureFilename(material, aiTextureType_DIFFUSE);
		auto normal_filename = GetTextureFilename(material, aiTextureType_HEIGHT);
		auto specular_filename = GetTextureFilename(material, aiTextureType_SPECULAR);

		aiColor3D diffuse_color, specular_color;
		material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse_color);
		material->Get(AI_MATKEY_COLOR_SPECULAR, specular_color);

		diffuse_filename = directory + diffuse_filename;
		normal_filename = directory + normal_filename;
		specular_filename = directory + specular_filename;

		DebugMaterialInfo info = {};
		info.diffuse_filename = diffuse_filename.c_str();
		info.specular_filename =  specular_filename.c_str();
		info.normal_filename = normal_filename.c_str();
		info.diffuse_color = V3(diffuse_color.r, diffuse_color.g, diffuse_color.b);
		info.specular_color = V3(specular_color.r, specular_color.g, specular_color.b);
		CreateMaterialData(&data.materialDataPerMesh[i], &info, memblock);
	}
	return data;
}
#endif
