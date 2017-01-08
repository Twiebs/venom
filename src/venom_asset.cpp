
//NOTE(Torin 2017-01-07) At this point the model assset's state is AssetState_Loading
//and no other threads have access to the data in the slot.  Upon exiting this procedure
//the asset will still be in the loading state and requires a secondary clean up phase to
//generate the data for opengl.  This is probably a temporary measure!
static inline bool CreateModelAssetFromFile(U32 slot_index) {
  auto manifest = GetAssetManifest();
  AssetSlot *slot = &manifest->modelAssets[slot_index];
  SpinLock(&slot->lock);

  assert(slot->asset == 0);
  assert(slot->filename != 0);
  assert(slot->name != 0);

  char filename[1024] = {};
  memcpy(filename, VENOM_ASSET_DIRECTORY, sizeof(VENOM_ASSET_DIRECTORY) - 2);
  strcpy(filename + sizeof(VENOM_ASSET_DIRECTORY) - 2, slot->filename);
  slot->lastWriteTime = GetFileLastWriteTime(filename);

  //TODO(Torin) Way to many memory allocations are happening here
  //and in the subsequent procedures!  ImportModelAssetFromExternalFormat should probably
  //be made and do the allocation of the model asset and everything else required as well
  //all at the exact same time to mimic how it would work in a release build to reduce the
  //overall compexity of the system
  slot->asset = malloc(sizeof(ModelAsset));
  memset(slot->asset, 0x00, sizeof(ModelAsset));
  ModelAsset* modelAsset = (ModelAsset *)slot->asset;
  if (ImportExternalModelData(filename, &modelAsset->data) == false) {
    slot->asset = 0;
    //TODO(Torin) How should this be set!
    assert(slot->assetState == AssetState_Loading);
    slot->assetState = AssetState_Invalid;
    return false;
  }

  modelAsset->slotIndex = slot_index;
  modelAsset->aabb = ComputeAABB(&modelAsset->data.meshData);
  modelAsset->size = Abs(modelAsset->aabb.max - modelAsset->aabb.min);
  modelAsset->drawable.materials = (MaterialDrawable *)calloc(modelAsset->data.meshCount, sizeof(MaterialDrawable));

  ModelDrawable *drawable = &modelAsset->drawable;
  drawable->indexCountPerMesh = modelAsset->data.index_count_per_mesh;
  drawable->vertexArrayID = modelAsset->vertexArray.vertexArrayID;
  drawable->meshCount = modelAsset->data.meshCount;
  drawable->joints = modelAsset->data.joints;
  drawable->joint_count = modelAsset->data.jointCount;
  slot->lastWriteTime = GetFileLastWriteTime(filename);

  //NOTE(Torin) The asset is still in the loading state and requires
  //additional processing after this procedure completes
  ReleaseLock(&slot->lock);
  return true;
}

static inline void CreateOpenGLResourcesForModelAsset(U32 slotIndex) {
  auto manifest = GetAssetManifest();
  auto slot = &manifest->modelAssets[slotIndex];
  SpinLock(&slot->lock);
  ModelAsset *modelAsset = (ModelAsset *)slot->asset;

  create_indexed_animated_vertex_array(&modelAsset->vertexArray, &modelAsset->data.meshData);
  for (size_t i = 0; i < modelAsset->data.meshCount; i++) {
    modelAsset->drawable.materials[i] = CreateMaterialDrawable(&modelAsset->data.materialDataPerMesh[i]);
  }

  assert(slot->assetState == AssetState_Loading);
  slot->assetState = AssetState_Loaded;
  ReleaseLock(&slot->lock);
}


void initalize_asset_manifest(AssetManifest *manifest) {
#ifndef VENOM_RELEASE
  //manifest->default_model_asset.


#endif//VENOM_RELEASE

  AssetSlot *slot = manifest->modelAssets.AddElement();
  slot->name = "null_model_asset";
  slot->filename = "/internal/null_model.fbx";
  slot->lastWriteTime = 0;
  CreateModelAssetFromFile(0);

  ReadAssetManifestFile("../project/assets.vsf", manifest);
}




#ifndef VENOM_RELEASE //===========================================================
//NOTE(Torin) The asset manifest is used to store asset information in
//development builds for easy runtime modifcation of asset data without
//using hardcoded values or requiring seperate metadata to be mantained


//TODO(Torin) This should be renamed or combined with a procedure
//that actualy modifes the asset slot to indicate the model is now unloaded
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
            LogDebug("Reloaded shader program");
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
    AssetSlot* modelSlot = &manifest->modelAssets[i];
    if (TryLock(&modelSlot->lock) == false) continue;
    AssetState assetState = (AssetState)manifest->modelAssets[i].assetState;
    if (assetState == AssetState_Loaded || assetState == AssetState_Invalid) {
      ModelAsset* modelAsset = (ModelAsset *)manifest->modelAssets[i].asset;
      strcpy(modelFilenameWrite, modelSlot->filename);
      U64 lastWriteTime = GetFileLastWriteTime(modelFilename);
      if (lastWriteTime != modelSlot->lastWriteTime) {
        modelSlot->lastWriteTime = lastWriteTime;
        if (manifest->modelAssets[i].assetState == AssetState_Invalid) {
          UnloadModelAsset(modelAsset);
        }

        manifest->modelAssets[i].assetState = AssetState_Unloaded;
        manifest->modelAssets[i].asset = 0;
        LogDebug("Unloaded model asset %s", modelSlot->name);
      }
    }
    ReleaseLock(&modelSlot->lock);
  }
}





void RemoveModelFromManifest(U32 index, AssetManifest *manifest) {
  AssetSlot *slot = &manifest->modelAssets[index];
  SpinLock(&slot->lock);
  if (slot->assetState == AssetState_Loaded) {
    UnloadModelAsset((ModelAsset *)slot->asset);
  }

  free(slot->name);
  free(slot->filename);
  //TODO(Torin) This is bad we need to lock model assets!
  manifest->modelAssets.RemoveUnordered(index);
  ReleaseLock(&slot->lock);
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

ModelAsset *GetModelAsset(Asset_ID& id) {
  AssetManifest *manifest = get_asset_manifest();
  return GetModelAsset(id, manifest);
}

ModelAsset* GetModelAsset(Asset_ID& id, AssetManifest* manifest) {
  if (manifest->modelReloadCounter != id.reload_counter_value)
    id = GetModelID(id.asset_name, manifest);

  AssetSlot* modelAssetSlot = &manifest->modelAssets[id.slot_index];
  if (TryLock(&modelAssetSlot->lock)) {
    if (modelAssetSlot->assetState == AssetState_Unloaded) {
      Task task;
      task.type = TaskType_LoadModel;
      task.slotID = id.slot_index;
      ScheduleTask(task);
      modelAssetSlot->assetState = AssetState_Loading;
      ReleaseLock(&modelAssetSlot->lock);
    } else if (modelAssetSlot->assetState == AssetState_Invalid) {
      Asset_ID null_id = {};
      return GetModelAsset(null_id, manifest);
    } else if (modelAssetSlot->assetState == AssetState_Loaded) {
      ModelAsset *modelAsset = (ModelAsset *)manifest->modelAssets[id.slot_index].asset;
      return modelAsset;
    }
  }

  return nullptr;
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