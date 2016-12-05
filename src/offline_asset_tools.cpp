#pragma clang diagnostic push 
#pragma clang diagnostic ignored "-Wall"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include "stb_vorbis.c"
#pragma clang diagnostic pop

#include "venom_utils.h"

SoundData LoadOGG(const char *filename) {
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

bool ImportExternalModelData(const char *filename, ModelData *data) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filename, aiProcess_JoinIdenticalVertices | aiProcess_FlipWindingOrder | 
    aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		LOG_ERROR("Assimp failed to load file (%s): %s", filename, importer.GetErrorString());
    return false;
	}


	data->meshCount = scene->mNumMaterials;
	for (U32 i = 0; i < scene->mNumMeshes; i++) {
		data->meshData.vertexCount += scene->mMeshes[i]->mNumVertices;
		data->meshData.indexCount += scene->mMeshes[i]->mNumFaces * 3;
	}

  size_t requiredMemory = sizeof(U32) * data->meshCount + sizeof(MaterialData) * data->meshCount +
    sizeof(AnimatedVertex) * data->meshData.vertexCount + sizeof(U32) * data->meshData.indexCount;

  //TODO(Torin) Make sure these are aligned correctly
	assert(data->meshCount == scene->mNumMaterials);
  uint8_t* memory = (uint8_t*)calloc(requiredMemory, 1);
  data->indexCountPerMesh = (U32*)memory;
  data->materialDataPerMesh = (MaterialData*)(data->indexCountPerMesh  + data->meshCount);
  data->meshData.vertices = (AnimatedVertex*)(data->materialDataPerMesh + data->meshCount);
  data->meshData.indices = (U32*)(data->meshData.vertices + data->meshData.vertexCount);

  //U32 total_bone_count = 0;

  size_t currentVertexOffset = 0;
  size_t currentIndexOffset = 0;
	for (size_t i = 0; i < scene->mNumMaterials; i++) {
		for (size_t n = 0; n < scene->mNumMeshes; n++) {
			if (scene->mMeshes[n]->mMaterialIndex == i) {
				aiMesh *assimpMesh = scene->mMeshes[n];

        //TODO(Torin) This might need to be fast get rid of the branches
        //Posibly change to inlined calls to static lambdas for setting each property
        //because we need to set these differently for each type of  vertex!
        //Mabye for now we should just use a single type of vertex and eat the memory wastage
        AnimatedVertex *vertices = (AnimatedVertex *)data->meshData.vertices;
        for (size_t j = 0; j < assimpMesh->mNumVertices; j++) {
          size_t index = currentVertexOffset + j;
          vertices[index].position.x = assimpMesh->mVertices[j].x;
          vertices[index].position.y = assimpMesh->mVertices[j].z;
          vertices[index].position.z = assimpMesh->mVertices[j].y;
          vertices[index].normal.x = assimpMesh->mNormals[j].x;
          vertices[index].normal.y = assimpMesh->mNormals[j].z;
          vertices[index].normal.z = assimpMesh->mNormals[j].y;
          if (assimpMesh->mTangents != nullptr) {
            vertices[index].tangent.x = assimpMesh->mTangents[j].x;
            vertices[index].tangent.y = assimpMesh->mTangents[j].z;
            vertices[index].tangent.z = assimpMesh->mTangents[j].y;
          }

          if (assimpMesh->mTextureCoords[0] != nullptr) {
            vertices[index].texcoord.x = assimpMesh->mTextureCoords[0][j].x;
            vertices[index].texcoord.y = assimpMesh->mTextureCoords[0][j].y;
          }
        }



#if 0
        for (size_t bone_index = 0; bone_index < assimpMesh->mNumBones; bone_index++) {
          aiBone *bone = assimpMesh->mBones[bone_index];
          for (size_t i = 0; i < bone->mNumWeights; i++) {
            aiVertexWeight *weight = &bone->mWeights[i];
            if (vertices[weight->mVertexId].bone_count >= 4) {
              LOG_ERROR("Vertex is influenced by too many bones: %s", filename);
              free(memory);
              return false;
            }

            //Does not account for total number of bones in the mesh!
            AnimatedVertex *vertex = &vertices[weight->mVertexId];
            vertex->bone_index[vertex->bone_count] = bone_index + total_bone_count;
            vertex->weight[vertex->bone_count] = weight->mWeight;
            vertex->bone_count++;
          }
          total_bone_count++;
        }
#endif

				U32 lastIndexOffset = currentIndexOffset;
				for (size_t j = 0; j < assimpMesh->mNumFaces; j++) {
					data->meshData.indices[currentIndexOffset + 0] = assimpMesh->mFaces[j].mIndices[0] + currentVertexOffset;
					data->meshData.indices[currentIndexOffset + 1] = assimpMesh->mFaces[j].mIndices[1] + currentVertexOffset;
					data->meshData.indices[currentIndexOffset + 2] = assimpMesh->mFaces[j].mIndices[2] + currentVertexOffset;
					currentIndexOffset += 3;
				}


				assert(currentIndexOffset == lastIndexOffset + (assimpMesh->mNumFaces * 3));
				currentVertexOffset += scene->mMeshes[n]->mNumVertices;
				data->indexCountPerMesh[i] += scene->mMeshes[n]->mNumFaces * 3;
			}
		}
	}

  auto GetTextureFilename = [&filename](MaterialTextureType type, char *out, aiMaterial *material) -> int { 
    const aiTextureType aiTextureTypeLookupTable[] = {
      aiTextureType_DIFFUSE,
      aiTextureType_NORMALS,
      aiTextureType_SPECULAR
    };

    size_t lastSlashOffset = LastOffsetOfChar('/', filename);
    memcpy(out, filename, lastSlashOffset + 1);
    char *textureFilenameWrite = out + lastSlashOffset + 1;

    size_t textureCount = material->GetTextureCount(aiTextureTypeLookupTable[type]); 
    if (textureCount == 0) return 0;
    if (textureCount > 1) {
      LogWarning("[LoadModel %s] Texture type %s specifies %d texture files."
        "Only one is supported and the others have been discarded",
        filename, MaterialTextureTypeNames[type], (int)textureCount);
    }
    
    aiString aiTextureFilename;
    material->GetTexture(aiTextureTypeLookupTable[type], 0, &aiTextureFilename);
    strcpy(textureFilenameWrite, aiTextureFilename.C_Str());
    return 1;
	};


	for (U32 materialIndex = 0; materialIndex < scene->mNumMaterials; materialIndex++) {
    
		aiMaterial* material = scene->mMaterials[materialIndex];
    char filenames[MaterialTextureType_COUNT][1024] = {};
    uint32_t materialFlags = 0;
    for(size_t i = 0; i < MaterialTextureType_COUNT; i++){
      int isFilenameValid = GetTextureFilename((MaterialTextureType)i, filenames[i], material);
      materialFlags |= ((1 << i) * isFilenameValid);
    }

    RGB8 diffuseColor = {};
    if(materialFlags == 0) {
      aiColor3D aiDiffuseColor;
      material->Get(AI_MATKEY_COLOR_DIFFUSE, aiDiffuseColor);
      diffuseColor = { (U8)(aiDiffuseColor.r * 255), (U8)(aiDiffuseColor.g * 255), (U8)(aiDiffuseColor.b * 255) };
    }
    
    const char *filenameList[] = {
      filenames[0],
      filenames[1],
      filenames[2],
    };
     
    if (CreateMaterialData(filenameList, materialFlags, &data->materialDataPerMesh[materialIndex], diffuseColor) == false) {
      return false;
    }
	}
	return true;
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
