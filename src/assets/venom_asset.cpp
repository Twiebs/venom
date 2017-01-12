
//NOTE(Torin 2017-01-07) At this point the model assset's state is AssetState_Loading
//and no other threads have access to the data in the slot.  Upon exiting this procedure
//the asset will still be in the loading state and requires a secondary clean up phase to
//generate the data for opengl.  This is probably a temporary measure!
static inline bool CreateModelAssetFromFile(U32 slot_index) {
  AssetSlot *slot = &GetAssetManifest()->modelAssets[slot_index];
  AquireLock(&slot->lock);

  assert(slot->assetState == AssetState_Loading);
  assert(slot->asset == 0);
  assert(slot->filename != 0);
  assert(slot->name != 0);

  char filename[1024] = {};
  memcpy(filename, VENOM_ASSET_DIRECTORY, sizeof(VENOM_ASSET_DIRECTORY) - 2);
  strcpy(filename + sizeof(VENOM_ASSET_DIRECTORY) - 2, slot->filename);
  slot->lastWriteTime = GetFileLastWriteTime(filename);

  slot->asset = (void *)CreateModelAssetFromExternalFormat(filename);
  if (slot->asset == nullptr) {
    slot->assetState = AssetState_Invalid;
    ReleaseLock(&slot->lock);
    return false;
  }

  ModelAsset *model = (ModelAsset *)slot->asset;
  model->aabb = ComputeAABB(model->vertices, model->vertexCount);
  model->size = Abs(model->aabb.max - model->aabb.min);
  slot->lastWriteTime = GetFileLastWriteTime(filename);

  //NOTE(Torin) The asset is still in the loading state and requires
  //additional processing after this procedure completes
  ReleaseLock(&slot->lock);
  return true;
}

//NOTE(Torin 2017-01-11) This procedure is always invoked by the main thread!
static inline void CreateOpenGLResourcesForModelAsset(U32 slotIndex) {
  auto manifest = GetAssetManifest();
  auto slot = &manifest->modelAssets[slotIndex];
  AquireLock(&slot->lock);

  ModelAsset *model = (ModelAsset *)slot->asset;
  create_indexed_animated_vertex_array(&model->vertexArray.vertexArrayID, &model->vertexArray.vertexBufferID, &model->vertexArray.indexBufferID,
    model->vertexCount, model->indexCount, model->vertices, model->indices, GL_STATIC_DRAW);
  for (size_t i = 0; i < model->meshCount; i++) {
    CreateMaterialOpenGLData(&model->materialDataPerMesh[i]);
  }

  assert(slot->assetState == AssetState_Loading);
  slot->assetState = AssetState_Loaded;
  ReleaseLock(&slot->lock);
}

//TODO(Torin 2017-01-11) Move this somewhere more suitable!
static ModelAsset *g_nullModelAsset;

void InitalizeAssetManifest(AssetManifest *manifest) {
  g_nullModelAsset = CreateModelAssetFromExternalFormat("../assets/internal/null_model.fbx");
  ReadAssetManifestFile("../project/assets.vsf", manifest);
}

//NOTE(Torin 2017-01-11) The caller is responcible for aquiring a releasing the lock
static inline void UnloadModelAsset(AssetSlot *slot) {
  assert(slot->lock.value == 1);
  assert(slot->assetState == AssetState_Loaded);
  ModelAsset *model = (ModelAsset *)slot->asset;
  DestroyIndexedVertexArray(&model->vertexArray);
  for (size_t i = 0; i < model->meshCount; i++)
    DestroyMaterialOpenGLData(&model->materialDataPerMesh[i]);
  MemoryFree(model);
  slot->assetState = AssetState_Unloaded;
  LogDebug("Unloaded model asset %s", slot->name);
}

void HotloadModifedAssets(AssetManifest *manifest) {
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


  //Hotload modifed model assets
  //TODO(Torin) All this code is really scarry!
  char modelFilename[1024] = {};
  memcpy(modelFilename, VENOM_ASSET_DIRECTORY, sizeof(VENOM_ASSET_DIRECTORY) - 1);
  char *modelFilenameWrite = modelFilename + sizeof(VENOM_ASSET_DIRECTORY) - 1;
  for (size_t i = 0; i < manifest->modelAssets.count; i++) {
    AssetSlot* slot = &manifest->modelAssets[i];
    AquireLock(&slot->lock);
    if (slot->assetState == AssetState_Loaded || slot->assetState == AssetState_Invalid) {
      strcpy(modelFilenameWrite, slot->filename);
      U64 lastWriteTime = GetFileLastWriteTime(modelFilename);
      if (lastWriteTime != slot->lastWriteTime) {
        slot->lastWriteTime = lastWriteTime;
        UnloadModelAsset(slot);
      }
    }
    ReleaseLock(&slot->lock);
  }
}

//TODO(Torin) This whole procedure is really dangerous.  It should
//be moved to a seprate section only for the editor/debug mode
//and processed at a known time.  If there is work to do(assets to remove, etc)
//then it forces all the worker threads to finish what they are doing removes the model,
//and then restarts the work queue so we don't have to deal with requiring a lock on the 
//entire asset manifest which would be absurd.  It's only a debug thing anyway!
void RemoveModelFromManifest(U32 index, AssetManifest *manifest) {
  AssetSlot *slot = &manifest->modelAssets[index];
  AquireLock(&slot->lock);
  if (slot->assetState == AssetState_Loaded) {
    UnloadModelAsset(slot);
  }

  MemoryFree(slot->name);
  MemoryFree(slot->filename);
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
  result.slot_index = INVALID_U32;
  result.reload_counter_value = manifest->modelReloadCounter;
  return result;
}

ModelAsset *GetModelAsset(Asset_ID& id) {
  AssetManifest *manifest = get_asset_manifest();
  return GetModelAsset(id, manifest);
}

ModelAsset* GetModelAsset(Asset_ID& id, AssetManifest* manifest) {
  if (manifest->modelReloadCounter != id.reload_counter_value)
    id = GetModelID(id.asset_name, manifest);

  if (id.slot_index == INVALID_U32) {
    return g_nullModelAsset;
  }

  AssetSlot *slot = &manifest->modelAssets[id.slot_index];
  if (TryLock(&slot->lock)) {
    if (slot->assetState == AssetState_Unloaded) {
      Task task;
      task.type = TaskType_LoadModel;
      task.slotID = id.slot_index;
      ScheduleTask(task);
      slot->assetState = AssetState_Loading;
      ReleaseLock(&slot->lock);
    } else if (slot->assetState == AssetState_Invalid) {
      Asset_ID null_id = {};
      ReleaseLock(&slot->lock);
      return GetModelAsset(null_id, manifest);
    } else if (slot->assetState == AssetState_Loaded) {
      ModelAsset *modelAsset = (ModelAsset *)manifest->modelAssets[id.slot_index].asset;
      ReleaseLock(&slot->lock);
      return modelAsset;
    } else {
      ReleaseLock(&slot->lock);
      return nullptr;
    }
  }


  return nullptr;
}

#if 0
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
#endif

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