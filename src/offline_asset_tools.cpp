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
	const aiScene* scene = importer.ReadFile(filename, 0);

	if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		LOG_ERROR("Assimp failed to load file (%s): %s", filename, importer.GetErrorString());
    return false;
	}


	data->meshCount = scene->mNumMaterials;
	for (U32 i = 0; i < scene->mNumMeshes; i++) {
		data->meshData.vertexCount += scene->mMeshes[i]->mNumVertices;
		data->meshData.indexCount += scene->mMeshes[i]->mNumFaces * 3;
    data->meshData.boneCount += scene->mMeshes[i]->mNumBones;
	}

  size_t requiredMemory = sizeof(U32) * data->meshCount + 
    sizeof(MaterialData) * data->meshCount + sizeof(U32) * data->meshData.indexCount;
  requiredMemory += sizeof(AnimatedVertex) * data->meshData.vertexCount + (sizeof(Animation_Bone)*data->meshData.boneCount);

  //TODO(Torin) Make sure these are aligned correctly
	assert(data->meshCount == scene->mNumMaterials);
  uint8_t* memory = (uint8_t*)calloc(requiredMemory, 1);
  data->indexCountPerMesh = (U32*)memory;
  data->materialDataPerMesh = (MaterialData*)(data->indexCountPerMesh  + data->meshCount);
  data->meshData.vertices = (AnimatedVertex*)(data->materialDataPerMesh + data->meshCount);
  data->meshData.indices = (U32*)(data->meshData.vertices + data->meshData.vertexCount);
  if (data->meshData.boneCount > 0) {
    data->meshData.bones = (Animation_Bone *)(data->meshData.indices + data->meshData.indexCount);
  }

  size_t total_bone_count = 0;
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

          for (size_t bone_index = 0; bone_index < 4; bone_index++) {
            vertices[index].bone_index[bone_index] = -1;
            vertices[index].weight[bone_index] = 0.0f;
          }
        }

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

          assert(bone->mName.length < 64);
          memcpy(data->meshData.bones[total_bone_count].name, bone->mName.data, bone->mName.length);
          M4 &offset_matrix = data->meshData.bones[total_bone_count].offset_matrix;
#if 1
          offset_matrix[0][0] = bone->mOffsetMatrix.a1;
          offset_matrix[1][0] = bone->mOffsetMatrix.a2;
          offset_matrix[2][0] = bone->mOffsetMatrix.a3;
          offset_matrix[3][0] = bone->mOffsetMatrix.a4;
          offset_matrix[0][1] = bone->mOffsetMatrix.c1;
          offset_matrix[1][1] = bone->mOffsetMatrix.c2;
          offset_matrix[2][1] = bone->mOffsetMatrix.c3;
          offset_matrix[3][1] = bone->mOffsetMatrix.c4;
          offset_matrix[0][2] = bone->mOffsetMatrix.b1;
          offset_matrix[1][2] = bone->mOffsetMatrix.b2;
          offset_matrix[2][2] = bone->mOffsetMatrix.b3;
          offset_matrix[3][2] = bone->mOffsetMatrix.b4;
          offset_matrix[0][3] = bone->mOffsetMatrix.d1;
          offset_matrix[1][3] = bone->mOffsetMatrix.d2;
          offset_matrix[2][3] = bone->mOffsetMatrix.d3;
          offset_matrix[3][3] = bone->mOffsetMatrix.d4;
#else
          offset_matrix[0][0] = bone->mOffsetMatrix.a1;
          offset_matrix[0][1] = bone->mOffsetMatrix.a2;
          offset_matrix[0][2] = bone->mOffsetMatrix.a3;
          offset_matrix[0][3] = bone->mOffsetMatrix.a4;
          offset_matrix[1][0] = bone->mOffsetMatrix.b1;
          offset_matrix[1][1] = bone->mOffsetMatrix.b2;
          offset_matrix[1][2] = bone->mOffsetMatrix.b3;
          offset_matrix[1][3] = bone->mOffsetMatrix.b4;
          offset_matrix[2][0] = bone->mOffsetMatrix.c1;
          offset_matrix[2][1] = bone->mOffsetMatrix.c2;
          offset_matrix[2][2] = bone->mOffsetMatrix.c3;
          offset_matrix[2][3] = bone->mOffsetMatrix.c4;
          offset_matrix[3][0] = bone->mOffsetMatrix.d1;
          offset_matrix[3][1] = bone->mOffsetMatrix.d2;
          offset_matrix[3][2] = bone->mOffsetMatrix.d3;
          offset_matrix[3][3] = bone->mOffsetMatrix.d4;
#endif
          total_bone_count++;
        }

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