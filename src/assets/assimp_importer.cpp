#pragma clang diagnostic push 
#pragma clang diagnostic ignored "-Wall"
#undef malloc
#undef free
#undef realloc
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/scene.h>
#define malloc(size) static_assert(false, "USE CUSTOM ALLOCATORS");
#define free(ptr) static_assert(false, "USE CUSTOM ALLOCATORS");
#define realloc(ptr, size) static_assert(false, "USE CUSTOM ALLOCATORS");
#pragma clang diagnostic pop

static inline M4 AssimpMatrixToM4(const aiMatrix4x4& m) {
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

static S32 RecursivlyChangeJointOrder(S32 oldJointIndex, Animation_Joint *jointList, Animation_Joint *tempJoints, size_t *currentTempJointCount) {
  S32 newJointIndex = *currentTempJointCount;
  *currentTempJointCount += 1;
  tempJoints[newJointIndex] = jointList[oldJointIndex];
  Animation_Joint *newJoint = &tempJoints[newJointIndex];

  S32 lastChildIndex = -1;
  S32 oldChildIndex = newJoint->child_index;
  newJoint->child_index = -1;

  while (oldChildIndex != -1) {
    S32 newChildIndex = RecursivlyChangeJointOrder(oldChildIndex, jointList, tempJoints, currentTempJointCount);
    Animation_Joint *newChild = &tempJoints[newChildIndex];
    newChild->parent_index = newJointIndex;
    if (lastChildIndex != -1) {
      Animation_Joint *lastChild = &tempJoints[lastChildIndex];
      lastChild->sibling_index = newChildIndex;
    }

    if (newJoint->child_index == -1) {
      newJoint->child_index = newChildIndex;
    }

    oldChildIndex = newChild->sibling_index;
    lastChildIndex = newChildIndex;
  }

  return newJointIndex;
}

AnimationType GetAnimationTypeFromClipName(const char *name) {
  if (CStringContainsCSubstringCaseInsensitive("idle", name)) return AnimationType_Idle;
  if (CStringContainsCSubstringCaseInsensitive("walk", name)) return AnimationType_Walk;
  return AnimationType_Invalid;
}


ModelAsset *CreateModelAssetFromExternalFormat(const char *filename) {
  Assimp::Importer importer;
  U32 flags = aiProcess_LimitBoneWeights |
    aiProcess_RemoveRedundantMaterials |
    aiProcess_JoinIdenticalVertices |
    aiProcess_SortByPType |
    aiProcess_Triangulate;

  const aiScene* scene = importer.ReadFile(filename, flags);
  if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    LogError("Assimp failed to load file (%s): %s", filename, importer.GetErrorString());
    return false;
  }

  assert(scene->mNumMaterials == scene->mNumMeshes);
  U32 tempMeshCount = scene->mNumMaterials;
  U32 tempAnimationClipCount = scene->mNumAnimations;  
  U32 tempVertexCount = 0;
  U32 tempIndexCount = 0;
  U32 tempJointCount = 0;
  DynamicArray<aiNode *> joint_ptrs;
  for (size_t i = 0; i < scene->mNumMeshes; i++) {
    aiMesh *mesh = scene->mMeshes[i];
    if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) continue;;
    tempVertexCount += mesh->mNumVertices;
    tempIndexCount += mesh->mNumFaces * 3;
    for (size_t j = 0; j < mesh->mNumBones; j++) {
      aiBone *bone = mesh->mBones[j];
      aiNode *bone_node = scene->mRootNode->FindNode(bone->mName);
      assert(bone_node != nullptr);
      if (!joint_ptrs.ContainsValue(bone_node)) {
        joint_ptrs.PushBack(bone_node);
      }
    }
  }

  tempJointCount = joint_ptrs.count;
  if (tempJointCount > 254) {
    LogError("Only 254 bones per mesh are premitted");
    return false;
  }

  size_t required_memory = 0;
  required_memory += Align8(sizeof(ModelAsset));                              //ModelAsset
  required_memory += Align8(sizeof(AnimatedVertex) *tempVertexCount);         //vertices
  required_memory += Align8(sizeof(U32) * tempIndexCount);                    //indices
  required_memory += Align8(sizeof(U32) * tempMeshCount);                     //indexCountPerMesh
  required_memory += Align8(sizeof(MaterialData) * tempMeshCount);            //materialDataPerMesh
  required_memory += Align8(sizeof(Animation_Joint) * tempJointCount);        //joints
  required_memory += Align8(sizeof(Animation_Clip) * tempAnimationClipCount); //AnimationClips
  
  ModelAsset *model = (ModelAsset *)MemoryAllocate(required_memory);
  memset(model, 0x00, required_memory);
  model->totalAssetMemorySize = required_memory;
  model->vertexCount          = tempVertexCount;
  model->indexCount           = tempIndexCount;
  model->jointCount           = tempJointCount;
  model->animationClipCount   = tempAnimationClipCount;
  model->meshCount            = tempMeshCount;
  model->indexCountPerMesh    = (U32 *)Align8(model + 1);
  model->materialDataPerMesh  = (MaterialData*)Align8(model->indexCountPerMesh + model->meshCount);
  model->vertices             = (AnimatedVertex*)Align8(model->materialDataPerMesh + model->meshCount);
  model->indices              = (U32*)Align8(model->vertices + model->vertexCount);
  if (model->jointCount > 0) {
    model->joints         = (Animation_Joint *)Align8(model->indices + model->indexCount);
    model->animationClips = (Animation_Clip *)Align8(model->joints + model->jointCount);
  }

  //TODO(Torin) Change these to U8s and make invalid index 0xFF
  for (size_t i = 0; i < model->jointCount; i++) {
    Animation_Joint *joint = &model->joints[i];
    memcpy(joint->name, "INVALID", sizeof("INVALID"));
    joint->parent_index = -1;
    joint->child_index = -1;
    joint->sibling_index = -1;
  }

  auto get_joint_index = [model](const char *name) -> S32 {
    for (size_t i = 0; i < model->jointCount; i++) {
      if (cstrings_are_equal(model->joints[i].name, name)) {
        return (S32)i;
      }
    }
    return -1;
  };

  M4 root_transform = AssimpMatrixToM4(scene->mRootNode->mTransformation);
  M4 inverseRootTransform = Inverse(root_transform);

  M4 armatureTransform = M4Identity();

  size_t current_joint_count = 0;
  size_t currentVertexOffset = 0;
  size_t currentIndexOffset = 0;

  { //NOTE(Torin) Create models joints and arange them in the proper order

    //NOTE(Torin) First the bones are created and stored in our data structure
    //This step is done first because the bone hiearcy needs to be reordered
    //so that it forms a flattened tree.

    //TODO(Torin) Make sure there are no bones with duplicate names!
    for (size_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
      aiMesh *mesh = scene->mMeshes[meshIndex];
      for (size_t i = 0; i < mesh->mNumBones; i++) {
        aiBone *bone = mesh->mBones[i];
        aiNode *boneNode = scene->mRootNode->FindNode(bone->mName);
        if (bone->mName.length > 63) {
          LogError("Model contains bones with names larger than 64 chars");
          MemoryFree(model);
          return false;
        }

        S32 jointIndex = get_joint_index(bone->mName.data);
        if (jointIndex == -1) {
          Animation_Joint *joint = &model->joints[i];
          memcpy(joint->name, bone->mName.data, bone->mName.length + 1);
          joint->localTransform = AssimpMatrixToM4(boneNode->mTransformation);
        }

      }
    }

    for (size_t i = 0; i < model->jointCount; i++) {
      Animation_Joint *joint = &model->joints[i];
      aiNode *bone_node = scene->mRootNode->FindNode(joint->name);
      aiNode *parent = bone_node->mParent;
      S32 parentIndex = get_joint_index(parent->mName.data);
      joint->parent_index = parentIndex;

      S32 last_child_index = -1;

      for (size_t c = 0; c < bone_node->mNumChildren; c++) {
        aiNode *ai_child = bone_node->mChildren[c];
        S32 child_index = get_joint_index(ai_child->mName.data);

        if (child_index == -1) {
          //NOTE(Torin) This will happen because .fbx will export a bone_end node
          assert(bone_node->mNumChildren == 1);
          continue;
        }

        joint->child_index = child_index;

        Animation_Joint *child_joint = &model->joints[child_index];
        child_joint->parent_index = i;
        if (last_child_index != -1) {
          Animation_Joint *previous_child = &model->joints[last_child_index];
          previous_child->sibling_index = child_index;
        }
        last_child_index = child_index;
      }
    }

    //TODO(Torin) Temporary worker memory
    size_t currentTempJointCount = 0;
    Animation_Joint *tempJoints = (Animation_Joint *)MemoryAllocate(model->jointCount * sizeof(Animation_Joint));
    memset(tempJoints, 0x00, model->jointCount * sizeof(Animation_Joint));
    for (size_t i = 0; i < model->jointCount; i++) {
      if (model->joints[i].parent_index == -1) {
        RecursivlyChangeJointOrder(i, model->joints, tempJoints, &currentTempJointCount);
      }
    }

    assert(currentTempJointCount == model->jointCount);
    memcpy(model->joints, tempJoints, sizeof(Animation_Joint) * model->jointCount);
    MemoryFree(tempJoints);


    { //Make each root joint a sibling of the previous root joint
      Animation_Joint *current_parent = &model->joints[0];
      for (size_t i = 1; i < model->jointCount; i++) {
        Animation_Joint *joint = &model->joints[i];
        if (joint->parent_index == -1) {
          current_parent->sibling_index = i;
          current_parent = joint;
        }
      }
    }

    //TODO(Torin) Invert root xform... Or all of them apparently?
    for (size_t i = 0; i < model->jointCount; i++) {
      if (model->joints[i].parent_index == -1) {

      }
    }

    for (size_t i = 0; i < model->jointCount; i++) {
      Animation_Joint *joint = &model->joints[i];
      if (joint->parent_index == -1) {
        aiNode *boneNode = scene->mRootNode->FindNode(joint->name);
        aiNode *parent = boneNode->mParent;
        assert(parent != nullptr);
        armatureTransform = AssimpMatrixToM4(parent->mTransformation);
        //joint->localTransform = root_transform * joint->localTransform;
        joint->localTransform = armatureTransform * joint->localTransform;
        joint->globalTransform = joint->localTransform;
      } else {
        Animation_Joint *parent = &model->joints[joint->parent_index];
        joint->globalTransform = parent->globalTransform * joint->localTransform;
      }
      joint->inverseBindPose = Inverse(joint->globalTransform);
    }
  } //============================



  for (size_t material_index = 0; material_index < scene->mNumMaterials; material_index++) {
    for (size_t n = 0; n < scene->mNumMeshes; n++) {
      if (scene->mMeshes[n]->mMaterialIndex == material_index) {
        aiMesh *assimpMesh = scene->mMeshes[n];
        aiNode *meshNode = scene->mRootNode->FindNode(assimpMesh->mName);
        if (meshNode == nullptr) {
          LogError("[Assimp Importer] Mesh name does not match parent object!");
          MemoryFree(model);
          return false;
        } 

        M4 transformMatrix = root_transform * AssimpMatrixToM4(meshNode->mTransformation);

        { //NOTE(Torin) Process the meshes vertices
          AnimatedVertex *vertices = model->vertices + currentVertexOffset;
          for (size_t i = 0; i < assimpMesh->mNumVertices; i++) {
            vertices[i].position.x = assimpMesh->mVertices[i].x;
            vertices[i].position.y = assimpMesh->mVertices[i].y;
            vertices[i].position.z = assimpMesh->mVertices[i].z;
            vertices[i].normal.x = assimpMesh->mNormals[i].x;
            vertices[i].normal.y = assimpMesh->mNormals[i].y;
            vertices[i].normal.z = assimpMesh->mNormals[i].z;
            vertices[i].position = V3(transformMatrix * V4(vertices[i].position, 1.0));
            vertices[i].normal = V3(transformMatrix * V4(vertices[i].normal, 0.0));
            if (assimpMesh->mTangents != nullptr) {
              vertices[i].tangent.x = assimpMesh->mTangents[i].x;
              vertices[i].tangent.y = assimpMesh->mTangents[i].y;
              vertices[i].tangent.z = assimpMesh->mTangents[i].z;
              vertices[i].tangent = V3(transformMatrix * V4(vertices[i].tangent, 0.0));
            }

            if (assimpMesh->mTextureCoords[0] != nullptr) {
              //TODO(Torin) How do we check if these need to be fliped?
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
          U32 *indices = model->indices + currentIndexOffset;
          for (size_t j = 0; j < assimpMesh->mNumFaces; j++) {
            indices[(j * 3) + 0] = assimpMesh->mFaces[j].mIndices[0] + currentVertexOffset;
            indices[(j * 3) + 1] = assimpMesh->mFaces[j].mIndices[1] + currentVertexOffset;
            indices[(j * 3) + 2] = assimpMesh->mFaces[j].mIndices[2] + currentVertexOffset;
          }

          model->indexCountPerMesh[material_index] += scene->mMeshes[n]->mNumFaces * 3;
        }


        { //NOTE(Torin) Now we process the joints for each mesh in the model
          for (size_t i = 0; i < assimpMesh->mNumBones; i++) {
            aiBone *bone = assimpMesh->mBones[i];
            S32 joint_index = get_joint_index(bone->mName.data);
            assert(joint_index != -1);

            for (size_t j = 0; j < bone->mNumWeights; j++) {
              aiVertexWeight *weight = &bone->mWeights[j];
              AnimatedVertex *vertex = &model->vertices[weight->mVertexId + currentVertexOffset];

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
                MemoryFree(model);
                return false;
              }

              assert(vertex_joint_index <= i);

              if (vertex->weight[vertex_joint_index] != 0) {
                if (vertex->weight[vertex_joint_index] != weight->mWeight) {
                  LogError("FUCK FUCK SHIT BALLS ASS");
                }
              }
              vertex->weight[vertex_joint_index] = weight->mWeight;
            }
          }
        }

        currentVertexOffset += scene->mMeshes[n]->mNumVertices;
        currentIndexOffset += scene->mMeshes[n]->mNumFaces * 3;
      }
    }
  }

  for (size_t i = 0; i < scene->mNumAnimations; i++) {
    aiAnimation *animation = scene->mAnimations[i];
    Animation_Clip *clip = &model->animationClips[i];
    clip->durationInTicks = animation->mDuration;
    clip->ticksPerSecond = animation->mTicksPerSecond;
    clip->joint_count = animation->mNumChannels;
    memcpy(clip->name, animation->mName.data, animation->mName.length + 1);
    if (cstrings_are_equal(clip->name, "")) {
      LogError("Animation Clips must have a valid name!");
      MemoryFree(model);
      return nullptr;
    }

    clip->type = GetAnimationTypeFromClipName(clip->name);
    clip->joint_animations.Resize(clip->joint_count);

    for (size_t j = 0; j < animation->mNumChannels; j++) {
      aiNodeAnim *node_anim = animation->mChannels[j];
      Joint_Animation *joint_animation = &clip->joint_animations[j];
      joint_animation->joint_index = get_joint_index(node_anim->mNodeName.data);
      if (joint_animation->joint_index == -1) continue; 
      //NOTE(Torin) Happens if the fileformat is .fbx  
      //animation data is exported for the armature itself which is unnessacary!        
      //TODO(Torin) Make sure this animation is excluded from the animation clip count!

      joint_animation->translation_count = node_anim->mNumPositionKeys;
      joint_animation->rotation_count = node_anim->mNumRotationKeys;
      joint_animation->scaling_count = node_anim->mNumScalingKeys;
      joint_animation->translations.Resize(joint_animation->translation_count);
      joint_animation->rotations.Resize(joint_animation->rotation_count);
      joint_animation->scalings.Resize(joint_animation->scaling_count);
      Animation_Joint *joint = &model->joints[joint_animation->joint_index];

      for (size_t k = 0; k < joint_animation->translation_count; k++) {
        joint_animation->translations[k].time = node_anim->mPositionKeys[k].mTime;
        joint_animation->translations[k].translation.x = node_anim->mPositionKeys[k].mValue.x;
        joint_animation->translations[k].translation.y = node_anim->mPositionKeys[k].mValue.y;
        joint_animation->translations[k].translation.z = node_anim->mPositionKeys[k].mValue.z;
        if (joint->parent_index == -1) {
          M4 transform = armatureTransform * root_transform;
          joint_animation->translations[k].translation = V3(transform * V4(joint_animation->translations[k].translation, 1.0f));
        }
      }

      for (size_t k = 0; k < joint_animation->rotation_count; k++) {
        joint_animation->rotations[k].time = node_anim->mRotationKeys[k].mTime;
        joint_animation->rotations[k].rotation.x = node_anim->mRotationKeys[k].mValue.x;
        joint_animation->rotations[k].rotation.y = node_anim->mRotationKeys[k].mValue.y;
        joint_animation->rotations[k].rotation.z = node_anim->mRotationKeys[k].mValue.z;
        joint_animation->rotations[k].rotation.w = node_anim->mRotationKeys[k].mValue.w;
        if (joint->parent_index == -1) {
          M4 rotation_matrix = root_transform * QuaternionToMatrix4x4(joint_animation->rotations[k].rotation);
          rotation_matrix = armatureTransform * rotation_matrix;
          joint_animation->rotations[k].rotation = MatrixToQuaternion(rotation_matrix);
        }
      }

      for (size_t k = 0; k < joint_animation->scaling_count; k++) {
        joint_animation->scalings[k].time = node_anim->mScalingKeys[k].mTime;
        joint_animation->scalings[k].scale = node_anim->mScalingKeys[k].mValue.x;
        bool is_valid = Equals(node_anim->mScalingKeys->mValue.x, node_anim->mScalingKeys[k].mValue.y, 0.001);
        is_valid = is_valid && Equals(node_anim->mScalingKeys->mValue.x, node_anim->mScalingKeys[k].mValue.z, 0.001);
        if (is_valid == false) {
          LogError("Animation encodes a non-uniform scale keyframe!");
          MemoryFree(model);
          return nullptr;
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
    for (size_t i = 0; i < MaterialTextureType_COUNT; i++) {
      int isFilenameValid = GetTextureFilename((MaterialTextureType)i, filenames[i], material);
      materialFlags |= ((1 << i) * isFilenameValid);
    }

    RGB8 diffuseColor = {};
    if (materialFlags == 0) {
      aiColor3D aiDiffuseColor;
      material->Get(AI_MATKEY_COLOR_DIFFUSE, aiDiffuseColor);
      diffuseColor = { (U8)(aiDiffuseColor.r * 255), (U8)(aiDiffuseColor.g * 255), (U8)(aiDiffuseColor.b * 255) };
    }

    const char *filenameList[] = {
      filenames[0],
      filenames[1],
      filenames[2],
    };

    if (CreateMaterialData(filenameList, materialFlags, &model->materialDataPerMesh[materialIndex], diffuseColor) == false) {
      return nullptr;
    }
  }
  return model;
}