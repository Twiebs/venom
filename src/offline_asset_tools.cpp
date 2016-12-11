#pragma clang diagnostic push 
#pragma clang diagnostic ignored "-Wall"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include "stb_vorbis.c"
#pragma clang diagnostic pop


#include "venom_utils.h"
#include "mesh_utilites.cpp"

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

static inline M4 aiMatrix_to_M4(const aiMatrix4x4& m) {
  M4 r;
  r[0][0] = m.a1;
  r[1][0] = m.a2;
  r[2][0] = m.a3;
  r[3][0] = m.a4;
  r[0][1] = m.b1;
  r[1][1] = m.b2;
  r[2][1] = m.b3;
  r[3][1] = m.b4;
  r[0][2] = m.c1;
  r[1][2] = m.c2;
  r[2][2] = m.c3;
  r[3][2] = m.c4;
  r[0][3] = m.d1;
  r[1][3] = m.d2;
  r[2][3] = m.d3;
  r[3][3] = m.d4;
  return r;
}

bool ImportExternalModelData(const char *filename, ModelData *data) {
	Assimp::Importer importer;
  U32 flags = aiProcess_LimitBoneWeights | 
    aiProcess_RemoveRedundantMaterials | 
    aiProcess_JoinIdenticalVertices |
    aiProcess_SortByPType |
    aiProcess_Triangulate;

	const aiScene* scene = importer.ReadFile(filename, flags);
	if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		LOG_ERROR("Assimp failed to load file (%s): %s", filename, importer.GetErrorString());
    return false;
	}

  //TODO(Torin) Just change this to a material count
  assert(scene->mNumMaterials == scene->mNumMeshes);
	data->meshCount = scene->mNumMaterials;
  data->animation_clip_count = scene->mNumAnimations;
  DynamicArray<aiNode *> joint_ptrs;
	for (size_t i = 0; i < scene->mNumMeshes; i++) {
    aiMesh *mesh = scene->mMeshes[i];
    if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) continue;;
    data->meshData.vertexCount += mesh->mNumVertices;
		data->meshData.indexCount += mesh->mNumFaces * 3;
    for (size_t j = 0; j < mesh->mNumBones; j++) {
      aiBone *bone = mesh->mBones[j];
      aiNode *bone_node = scene->mRootNode->FindNode(bone->mName);
      assert(bone_node != nullptr);
      if (!joint_ptrs.ContainsValue(bone_node)) {
        joint_ptrs.PushBack(bone_node);
      }
    }
	}

  data->jointCount = joint_ptrs.count;
  if (data->jointCount > 254) {
    LOG_ERROR("Only 254 bones per mesh are premitted");
    return false;
  }

  //TODO(Torin) Make sure these are aligned correctly
  //TODO(Torin) Consider consolidating index count and bone
  //count per mesh into a single structure for simplicity
  size_t required_memory = 0;
  required_memory += sizeof(U32) * data->meshCount; //index count per mesh
  required_memory += sizeof(MaterialData) *data->meshCount;
  required_memory += sizeof(U32) * data->meshData.indexCount;
  required_memory += sizeof(AnimatedVertex) *data->meshData.vertexCount;
  required_memory += sizeof(U32) * data->meshCount;
  required_memory += sizeof(Animation_Joint) * data->jointCount;
  required_memory += sizeof(U32) * data->meshCount; //joint count per mesh
  required_memory += sizeof(Animation_Clip) * data->animation_clip_count;
	
  //TODO(Torin) Make this a call to a custom memory allocator
  uint8_t *memory = (uint8_t *)calloc(required_memory, 1);
  data->index_count_per_mesh = (U32 *)memory;
  data->materialDataPerMesh = (MaterialData*)(data->index_count_per_mesh + data->meshCount);
  data->meshData.vertices = (AnimatedVertex*)(data->materialDataPerMesh + data->meshCount);
  data->meshData.indices = (U32*)(data->meshData.vertices + data->meshData.vertexCount);
  if (data->jointCount > 0) {
    data->joints = (Animation_Joint *)(data->meshData.indices + data->meshData.indexCount);
    data->joint_count_per_mesh = (U32 *)(data->joints + data->jointCount);
    data->animation_clips = (Animation_Clip *)(data->joint_count_per_mesh + data->meshCount);
  }

  //TODO(Torin) Change these to U8s and make invalid index 0xFF
  for (size_t i = 0; i < data->jointCount; i++) {
    Animation_Joint *joint = &data->joints[i];
    memcpy(joint->name, "INVALID", sizeof("INVALID"));
    joint->parent_index = -1;
    joint->child_index = -1;
    joint->sibling_index = -1;
  }

  auto get_joint_index = [data](const char *name) -> S32 {
    for (size_t i = 0; i < data->jointCount; i++) {
      if (cstrings_are_equal(data->joints[i].name, name)) {
        return (S32)i;
      }
    }
    return -1;
  };

  size_t current_joint_count = 0;
  size_t currentVertexOffset = 0;
  size_t currentIndexOffset = 0;
  for (size_t material_index = 0; material_index < scene->mNumMaterials; material_index++) {
    for (size_t n = 0; n < scene->mNumMeshes; n++) {
      if (scene->mMeshes[n]->mMaterialIndex == material_index) {
        aiMesh *assimpMesh = scene->mMeshes[n];

      //TODO(Torin) This might need to be fast get rid of the branches
      //Posibly change to inlined calls to static lambdas for setting each property
      //because we need to set these differently for each type of  vertex!
      //Mabye for now we should just use a single type of vertex and eat the memory wastage
      { //NOTE(Torin) Process the meshes vertices
        AnimatedVertex *vertices = data->meshData.vertices + currentVertexOffset;
        for (size_t i = 0; i < assimpMesh->mNumVertices; i++) {
          vertices[i].position.x = assimpMesh->mVertices[i].x;
          vertices[i].position.y = assimpMesh->mVertices[i].y;
          vertices[i].position.z = assimpMesh->mVertices[i].z;
          vertices[i].normal.x = assimpMesh->mNormals[i].x;
          vertices[i].normal.y = assimpMesh->mNormals[i].y;
          vertices[i].normal.z = assimpMesh->mNormals[i].z;
          if (assimpMesh->mTangents != nullptr) {
            vertices[i].tangent.x = assimpMesh->mTangents[i].x;
            vertices[i].tangent.y = assimpMesh->mTangents[i].y;
            vertices[i].tangent.z = assimpMesh->mTangents[i].z;
          }
          if (assimpMesh->mTextureCoords[0] != nullptr) {
            vertices[i].texcoord.x = assimpMesh->mTextureCoords[0][i].x;
            vertices[i].texcoord.y = assimpMesh->mTextureCoords[0][i].y;
          }
          for (size_t j = 0; j < 4; j++) {
            vertices[i].joint_index[j] = -1;
            vertices[i].weight[j] = 0.0f;
          }
        }
      }

      { //NOTE(Torin) Now the indices are processed and added to the model data
        U32 *indices = data->meshData.indices + currentIndexOffset;
        for (size_t j = 0; j < assimpMesh->mNumFaces; j++) {
          indices[(j*3) + 0] = assimpMesh->mFaces[j].mIndices[0] + currentVertexOffset;
          indices[(j*3) + 1] = assimpMesh->mFaces[j].mIndices[1] + currentVertexOffset;
          indices[(j*3) + 2] = assimpMesh->mFaces[j].mIndices[2] + currentVertexOffset;
        }

        data->index_count_per_mesh[material_index] += scene->mMeshes[n]->mNumFaces * 3;
      }



      
      { //NOTE(Torin) Now we process the joints for each mesh in the model
        for (size_t i = 0; i < assimpMesh->mNumBones; i++) {
          aiBone *bone = assimpMesh->mBones[i];
          if (bone->mName.length > 63) {
            LOG_ERROR("Model contains bones with names larger than 64 chars");
            free(data);
            return false;
          }

          aiNode *bone_node = scene->mRootNode->FindNode(bone->mName);
          S32 joint_index = get_joint_index(bone->mName.data);
          if (joint_index == -1) {
            joint_index = current_joint_count++;
            Animation_Joint *joint = &data->joints[joint_index];
            memcpy(joint->name, bone->mName.data, bone->mName.length+1);
            joint->bind_pose_matrix = aiMatrix_to_M4(bone->mOffsetMatrix);
            joint->inverse_bind_matrix = Inverse(joint->bind_pose_matrix);
            joint->parent_realtive_matrix = aiMatrix_to_M4(bone_node->mTransformation);
            data->joint_count_per_mesh[material_index]++;   //XXX What's this purpose noww
          }

          for (size_t j = 0; j < bone->mNumWeights; j++) {
            aiVertexWeight *weight = &bone->mWeights[j];
            AnimatedVertex *vertex = &data->meshData.vertices[weight->mVertexId + currentVertexOffset];

            U8 vertex_joint_index = 0xFF;
            for (size_t i = 0; i < 4; i++) {
              if (vertex->joint_index[i] == joint_index) {
                vertex_joint_index = i;
              }

              if (vertex->joint_index[i] == -1) {
                vertex->joint_index[i] = joint_index;
                vertex_joint_index = i;
                break;
              }
            }

            if (vertex_joint_index == 0xFF) {
              LogError("More than 4 joints influences this vertex!");
              free(memory);
              return false;
            }

            assert(vertex_joint_index <= i);

            if (vertex->weight[vertex_joint_index] != 0) {
              if (vertex->weight[vertex_joint_index] != weight->mWeight) {
                LOG_ERROR("FUCK FUCK SHIT BALLS ASS");
              }
            }
            vertex->weight[vertex_joint_index] = weight->mWeight;
          }
        }
      }


      for (size_t i = 0; i < data->jointCount; i++) {
        Animation_Joint *joint = &data->joints[i];
        aiNode *bone_node = scene->mRootNode->FindNode(joint->name);
        S32 last_child_index = -1;
        for (size_t c = 0; c < bone_node->mNumChildren; c++) {
          aiNode *ai_child = bone_node->mChildren[c];

          S32 child_index = get_joint_index(ai_child->mName.data);
          if (child_index == -1) continue;
          if (joint->child_index == -1) {
            joint->child_index = child_index;
          }

          Animation_Joint *child_joint = &data->joints[child_index];
          child_joint->parent_index = i;
          if (last_child_index != -1) {
            Animation_Joint *previous_child = &data->joints[last_child_index];
            previous_child->sibling_index = child_index;
          }
          last_child_index = child_index;
        }
      }

      if (data->jointCount > 0) {
        normalize_vertex_joint_weights(data->meshData.vertices, data->meshData.vertexCount);
      }
      


      currentVertexOffset += scene->mMeshes[n]->mNumVertices;
      currentIndexOffset += scene->mMeshes[n]->mNumFaces * 3;
			}
		}
	}




  

  for (size_t i = 0; i < scene->mNumAnimations; i++) {
    aiAnimation *animation = scene->mAnimations[i];
    Animation_Clip *clip = &data->animation_clips[i];
    clip->duration = animation->mDuration;
    clip->joint_count = animation->mNumChannels;
    memcpy(clip->name, animation->mName.data, animation->mName.length + 1);
    clip->joint_animations.Resize(clip->joint_count);
    
    for (size_t j = 0; j < animation->mNumChannels; j++) {
      aiNodeAnim *node_anim = animation->mChannels[j];
      Joint_Animation *joint_animation = &clip->joint_animations[j];
      joint_animation->joint_index = get_joint_index(node_anim->mNodeName.data);
      joint_animation->translation_count = node_anim->mNumPositionKeys;
      joint_animation->rotation_count = node_anim->mNumRotationKeys;
      joint_animation->scaling_count = node_anim->mNumScalingKeys;
      joint_animation->translations.Resize(joint_animation->translation_count);
      joint_animation->rotations.Resize(joint_animation->rotation_count);
      joint_animation->scalings.Resize(joint_animation->scaling_count);

      for (size_t k = 0; k < joint_animation->translation_count; k++) {
        joint_animation->translations[k].time = node_anim->mPositionKeys->mTime;
        joint_animation->translations[k].translation.x = node_anim->mPositionKeys->mValue.x;
        joint_animation->translations[k].translation.y = node_anim->mPositionKeys->mValue.y;
        joint_animation->translations[k].translation.z = node_anim->mPositionKeys->mValue.z;
      }

      for (size_t k = 0; k < joint_animation->rotation_count; k++) {
        joint_animation->rotations[k].time = node_anim->mRotationKeys->mTime;
        joint_animation->rotations[k].rotation.x = node_anim->mRotationKeys->mValue.x;
        joint_animation->rotations[k].rotation.y = node_anim->mRotationKeys->mValue.y;
        joint_animation->rotations[k].rotation.z = node_anim->mRotationKeys->mValue.z;
        joint_animation->rotations[k].rotation.w = node_anim->mRotationKeys->mValue.w;
      }

      for (size_t k = 0; k < joint_animation->scaling_count; k++) {
        joint_animation->scalings[k].time = node_anim->mScalingKeys->mTime;
        joint_animation->scalings[k].scale = node_anim->mScalingKeys->mValue.x;
        bool is_valid = Equals(node_anim->mScalingKeys->mValue.x, node_anim->mScalingKeys->mValue.y);
        is_valid = is_valid && Equals(node_anim->mScalingKeys->mValue.x, node_anim->mScalingKeys->mValue.z);
        if (is_valid == false) {
          LOG_ERROR("Animation encodes a non-uniform scale keyframe!");
          free(data);
          return false;
        }
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