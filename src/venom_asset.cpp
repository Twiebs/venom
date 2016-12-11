
#if 0
static inline bool check_model_asset_for_errors(ModelAsset *model){
  MeshData *mesh_data = &model->data.meshData;
  if (mesh_data->joints > 0) {
    for (size_t i = 0; i < mesh_data->vertexCount; i++) {
      AnimatedVertex *vertex = &mesh_data->vertices[i];
      float total_weight = 0.0f;
      for (size_t j = 0; j < 4; j++) {
        total_weight += vertex->weight[j];
      }

      if (abs(1.0 - total_weight) > 0.1) {
        LogWarning("Vertex weights do not sum to 1.0!");
        return false;
      }
    }
  }


  return true;
}

#endif


static inline bool CreateModelAssetFromFile(U32 slot_index, AssetManifest *manifest) {
  AssetSlot *slot = &manifest->modelAssets[slot_index];
  assert(slot->asset == 0);
  assert(slot->filename != 0);
  assert(slot->name != 0);

  char filename[1024] = {};
  memcpy(filename, VENOM_ASSET_DIRECTORY, sizeof(VENOM_ASSET_DIRECTORY) - 2);
  strcpy(filename + sizeof(VENOM_ASSET_DIRECTORY) - 2, slot->filename);
  slot->lastWriteTime = GetFileLastWriteTime(filename);

  slot->asset = malloc(sizeof(ModelAsset));
  memset(slot->asset, 0x00, sizeof(ModelAsset));
  ModelAsset* modelAsset = (ModelAsset *)slot->asset;
  if (ImportExternalModelData(filename, &modelAsset->data) == false) {
    slot->asset = 0;
    slot->flags |= AssetFlag_INVALID;
    return false;
  }

  modelAsset->slot_index = slot_index;
  modelAsset->aabb = ComputeAABB(&modelAsset->data.meshData);
  modelAsset->size = Abs(modelAsset->aabb.max - modelAsset->aabb.min);
  modelAsset->drawable.materials = (MaterialDrawable *)calloc(modelAsset->data.meshCount, sizeof(MaterialDrawable));
  create_indexed_animated_vertex_array(&modelAsset->vertexArray, &modelAsset->data.meshData);
  
  ModelDrawable *drawable = &modelAsset->drawable;
  drawable->indexCountPerMesh = modelAsset->data.index_count_per_mesh;
  drawable->jointCountPerMesh = modelAsset->data.joint_count_per_mesh;
  drawable->vertexArrayID = modelAsset->vertexArray.vertexArrayID;
  drawable->meshCount = modelAsset->data.meshCount;
  drawable->joints = modelAsset->data.joints;
  drawable->joint_count = modelAsset->data.jointCount;

  slot->lastWriteTime = GetFileLastWriteTime(filename);
  for (size_t i = 0; i < modelAsset->data.meshCount; i++) {
    modelAsset->drawable.materials[i] = CreateMaterialDrawable(&modelAsset->data.materialDataPerMesh[i]);
  }

  //check_model_asset_for_errors(modelAsset);

  slot->flags |= AssetFlag_LOADED;
}

void initalize_asset_manifest(AssetManifest *manifest) {
#ifndef VENOM_RELEASE
  //manifest->default_model_asset.


#endif//VENOM_RELEASE

  AssetSlot *slot = manifest->modelAssets.AddElement();
  slot->name = "null_model_asset";
  slot->filename = "/internal/null_model.fbx";
  slot->lastWriteTime = 0;
  CreateModelAssetFromFile(0, manifest);

  ReadAssetManifestFile("../project/assets.vsf", manifest);
}




#ifndef VENOM_RELEASE //===========================================================
//NOTE(Torin) The asset manifest is used to store asset information in
//development builds for easy runtime modifcation of asset data without
//using hardcoded values or requiring seperate metadata to be mantained

static inline void UnloadModelAsset(ModelAsset *modelAsset) {
  assert(modelAsset->data.meshData.indexCount != 0);
  DestroyModelData(&modelAsset->data);
  DestroyIndexedVertexArray(&modelAsset->vertexArray);
  for (size_t i = 0; i < modelAsset->drawable.meshCount; i++)
    DestroyMaterialDrawable(&modelAsset->drawable.materials[i]);
  free(modelAsset->drawable.materials);
  free(modelAsset);
}

void hotload_modified_assets(AssetManifest *manifest) {
  for (U32 i = 0; i < DEBUGShaderID_COUNT; i++) {
    DEBUGLoadedShader *loadedShader = manifest->loadedShaders + i;
    if (!loadedShader->is_loaded) continue;
    for (auto n = 0; n < 4; n++) {
      const char *filename = loadedShader->filenames[n];
      if (filename != nullptr) {
        U64 lastWriteTime = GetFileLastWriteTime(filename);
        if (lastWriteTime != loadedShader->lastWriteTimes[n]) {
          loadedShader->lastWriteTimes[0] =
            GetFileLastWriteTime(loadedShader->filenames[0]);
          loadedShader->lastWriteTimes[1] =
            GetFileLastWriteTime(loadedShader->filenames[1]);
          loadedShader->lastWriteTimes[2] =
            GetFileLastWriteTime(loadedShader->filenames[2]);
          GLuint newShader = DEBUGCreateShaderProgramFromFiles(loadedShader->filenames);
          if (newShader != 0) {
            glDeleteProgram(loadedShader->programHandle);
            loadedShader->programHandle =
              DEBUGCreateShaderProgramFromFiles(loadedShader->filenames);
            glDeleteProgram(newShader);
            LOG_DEBUG("Reloaded shader program");
          }
          break;
        }
      }
    }
  }


  char modelFilename[1024] = {};
  memcpy(modelFilename, VENOM_ASSET_DIRECTORY, sizeof(VENOM_ASSET_DIRECTORY) - 1);
  char *modelFilenameWrite = modelFilename + sizeof(VENOM_ASSET_DIRECTORY) - 1;
  for (size_t i = 0; i < manifest->modelAssets.count; i++) {
    if (manifest->modelAssets[i].flags & AssetFlag_LOADED || manifest->modelAssets[i].flags & AssetFlag_INVALID) {
      AssetSlot* modelSlot = &manifest->modelAssets[i];
      ModelAsset* modelAsset = (ModelAsset *)manifest->modelAssets[i].asset;
      strcpy(modelFilenameWrite, modelSlot->filename);
      U64 lastWriteTime = GetFileLastWriteTime(modelFilename);
      if (lastWriteTime != modelSlot->lastWriteTime) {
        modelSlot->lastWriteTime = lastWriteTime;
        if ((manifest->modelAssets[i].flags & AssetFlag_INVALID) == 0) {
          UnloadModelAsset(modelAsset);
        }

        manifest->modelAssets[i].flags = 0;
        manifest->modelAssets[i].asset = 0;
        LogDebug("Unloaded model asset %s", modelSlot->name);
      }
    }
  }
}





void RemoveModelFromManifest(U32 index, AssetManifest *manifest) {
  AssetSlot *slot = &manifest->modelAssets[index];
  if (slot->flags & AssetFlag_LOADED) {
    UnloadModelAsset((ModelAsset *)slot->asset);
  }

  free(slot->name);
  free(slot->filename);
  manifest->modelAssets.RemoveUnordered(index);
}

void WriteAssetManifestFile(const char *filename, AssetManifest *manifest){
  if(vs::BeginFileWrite(filename) == 0) return;
  for(size_t i = 1; i < manifest->modelAssets.count; i++) {
    AssetSlot *slot = &manifest->modelAssets[i];
    vs::BeginGroupWrite("slot");
    vs::WriteString("name", slot->name);
    vs::WriteString("filename", slot->filename);
    vs::EndGroupWrite();
  }
  vs::EndFileWrite();
}

void ReadAssetManifestFile(const char *filename, AssetManifest *manifest){
  if(vs::BeginFileRead(filename) == 0) return;
  while(vs::BeginGroupRead()) {
    char nameBuffer[256] = {};
    char filenameBuffer[256] = {};
    vs::ReadString("name", nameBuffer, sizeof(nameBuffer));
    vs::ReadString("filename", filenameBuffer, sizeof(filenameBuffer));
    AssetSlot *slot = manifest->modelAssets.AddElement();
    slot->name = strdup(nameBuffer);
    slot->filename = strdup(filenameBuffer);
    vs::EndGroupRead();
  }
  vs::EndFileRead();
}

bool manifest_contains_model(const char *name, AssetManifest *manifest, U32 *slot_index) {
  for (size_t i = 0; i < manifest->modelAssets.count; i++) {
    if (strcmp(name, manifest->modelAssets[i].name) == 0) {
      *slot_index = i;
      return true;
    }
  }
  return false;
} 


Asset_ID GetModelID(const char *name, AssetManifest *manifest) {
  if (strlen(name) > 64) assert(false);

  for (size_t i = 0; i < manifest->modelAssets.count; i++) {
    if (strcmp(name, manifest->modelAssets[i].name) == 0) {
      Asset_ID result = {};
      if(result.asset_name != name) strcpy(result.asset_name, name);
      result.slot_index = i;
      result.reload_counter_value = manifest->modelReloadCounter;
      return result;
    }
  }

  Asset_ID result = {};
  if(result.asset_name != name) strcpy(result.asset_name, name);
  result.slot_index = 0;
  result.reload_counter_value = manifest->modelReloadCounter;
  return result;
}


//====================================================================================
#endif//VENOM_RELEASE



ModelAsset* GetModelAsset(Asset_ID& id, AssetManifest* manifest) {
  if (manifest->modelReloadCounter != id.reload_counter_value)
    id = GetModelID(id.asset_name, manifest);

  AssetSlot* modelAssetSlot = &manifest->modelAssets[id.slot_index];
  if((modelAssetSlot->flags & AssetFlag_LOADED) == 0){
    if (modelAssetSlot->flags & AssetFlag_INVALID) {
      Asset_ID null_id = {};
      return GetModelAsset(null_id, manifest);
    }

    if (CreateModelAssetFromFile(id.slot_index, manifest) == false) {
      Asset_ID null_id = {};
      return GetModelAsset(null_id, manifest);
    }
  }

  ModelAsset *modelAsset = (ModelAsset *)manifest->modelAssets[id.slot_index].asset;
  return modelAsset;
}

//TODO(Torin) Check load state here
ModelDrawable *GetModelDrawableFromIndex(U32 modelIndex, AssetManifest *manifest) {
  ModelAsset *modelAsset = (ModelAsset *)manifest->modelAssets[modelIndex].asset;
  return &modelAsset->drawable;
}

ModelDrawable *GetModelDrawable(Asset_ID& id, AssetManifest* manifest) {
  ModelAsset* modelAsset = GetModelAsset(id, manifest);
  if (modelAsset == nullptr) return nullptr;
  ModelDrawable *drawable = &modelAsset->drawable;
  return drawable;
}

const MaterialDrawable& 
GetMaterial(U32 id, AssetManifest* manifest){
  MaterialAssetList* list = &manifest->materialAssetList;
  MaterialAsset* materialAsset = &list->materials[id];
  assert(id < list->materialCount);
	if(materialAsset->drawable.diffuse_texture_id == 0){
		CreateMaterialData(materialAsset->filenames, 
      materialAsset->flags, &materialAsset->data);
		materialAsset->drawable = CreateMaterialDrawable(&materialAsset->data);
	}
	return materialAsset->drawable;
}

GLuint GetShaderProgram(DEBUGShaderID shaderID, AssetManifest* manifest) {
	DEBUGLoadedShader *loadedShader = &manifest->loadedShaders[shaderID];
	if (loadedShader->programHandle == 0) {
		for (int i = 0; i < ShaderType_COUNT; i++) {
			loadedShader->filenames[i] = DEBUG_SHADER_INFOS[shaderID].filenames[i];
			loadedShader->lastWriteTimes[i] = GetFileLastWriteTime(loadedShader->filenames[i]);
		}

    //TODO(Torin: May 12, 2016) 
		loadedShader->programHandle = 
      DEBUGCreateShaderProgramFromFiles(loadedShader->filenames);
		loadedShader->is_loaded = true;
	}
	
	return loadedShader->programHandle;
}