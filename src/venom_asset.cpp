
#ifndef VENOM_RELEASE
//==========================================================================
//NOTE(Torin) The asset manifest is used to store asset information in
//development builds for easy runtime modifcation of asset data without
//using hardcoded values or requiring seperate metadata to be mantained

struct AssetManifestFileHeader {
  static const U64 VERIFICATION_NUMBER = ((U64)'T' << 56) & ((U64)'S' << 48) 
    & ((U64)'E' << 40) & ((U64)'F' << 32) & ((U64)'I' << 24) & 
    ((U64)'N' << 16) & ((U64)'A' << 8) & ((U64)'M');

  U64 verificationNumber;
  U64 modelEntryCount;

  U64 stringBlockOffset;
  U64 stringBlockSize;
  U64 stringBlockEntryCount;
} __attribute__((packed));

struct AssetManifestFileEntry {
  U64 assetFlags;
  U64 nameStringOffset;
  U64 filenameStringOffset;
} __attribute__((packed));


//TODO(Torin) dont like the idea of these mallocs here
//but for now it will suffice since this is a debug mechanisim

void DestroyModelAsset(U32 index, AssetManifest *manifest) {
  assert(index < manifest->modelAssets.count);
  AssetSlot *slot = &manifest->modelAssets[index];
  free(slot->name);
  free(slot->filename);
  manifest->modelAssets.RemoveOrdered(index);
}

void WriteAssetManifestFile(const char *filename, AssetManifest *manifest){
  if(vs::BeginFileWrite(filename) == 0) return;
  for(size_t i = 0; i < manifest->modelAssets.count; i++) {
    AssetSlot *slot = &manifest->modelAssets[i];
    vs::BeginGroupWrite("slot");
    vs::WriteString("name", slot->name);
    vs::WriteString("filename", slot->filename);
    vs::EndGroupWrite();
  }
  vs::EndFileWrite();
}


#if 1
void ReadAssetManifestFile(const char *filename, AssetManifest *manifest){
  if(vs::BeginFileRead(filename) == 0) return;
  while(vs::BeginGroupRead()) {
    char nameBuffer[256] = {};
    char filenameBuffer[256] = {};
    vs::ReadString("name", nameBuffer, sizeof(nameBuffer));
    vs::ReadString("filename", filenameBuffer, sizeof(filenameBuffer));
    AssetSlot slot = {};
    slot.name = strdup(nameBuffer);

    const char *filenameToCopy = filenameBuffer + sizeof(VENOM_ASSET_DIRECTORY) - 1;
    slot.filename = strdup(filenameToCopy);
    manifest->modelAssets.PushBack(slot);
    vs::EndGroupRead();
  }
  vs::EndFileRead();
}
#endif




S64 GetModelID(const char *name, AssetManifest *manifest){
  for(size_t i = 0; i < manifest->modelAssets.count; i++){
    if(strcmp(name, manifest->modelAssets[i].name) == 0) {
      return i;
    }
  }
  return -1;
}


//====================================================================================
#endif//VENOM_RELEASE

static inline
void UnloadModelAsset(ModelAsset* modelAsset){
  assert(modelAsset->data.meshData.indexCount != 0);
  DestroyModelData(&modelAsset->data);
  DestroyIndexedVertexArray(&modelAsset->vertexArray);
  fori(modelAsset->drawable.meshCount){
    DestroyMaterialDrawable(&modelAsset->drawable.materials[i]);
  }
  free(modelAsset);//TODO(Torin) This is a hack!!!
}

static void HotloadShaders(AssetManifest *assets) {
	for (U32 i = 0; i < DEBUGShaderID_COUNT; i++) {
		DEBUGLoadedShader *loadedShader = assets->loadedShaders + i;
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
}

static inline
void HotloadModels(AssetManifest* manifest){
  char modelFilename[1024] = {};
  memcpy(modelFilename, VENOM_ASSET_DIRECTORY, sizeof(VENOM_ASSET_DIRECTORY) -1);
  char *modelFilenameWrite = modelFilename + sizeof(VENOM_ASSET_DIRECTORY) - 1;

  for(size_t i = 0; i < manifest->modelAssets.count; i++){
    if(manifest->modelAssets[i].flags & AssetFlag_Loaded){
      AssetSlot* modelSlot = &manifest->modelAssets[i];
      ModelAsset* modelAsset = (ModelAsset *)manifest->modelAssets[i].asset;
      strcpy(modelFilenameWrite, modelSlot->filename); 
      U64 lastWriteTime = GetFileLastWriteTime(modelFilename);
      if(lastWriteTime != modelAsset->lastWriteTime){
        modelAsset->lastWriteTime = lastWriteTime;
        UnloadModelAsset(modelAsset);
        manifest->modelAssets[i].flags = 0;
        manifest->modelAssets[i].asset = 0;
        LogDebug("Unloaded model asset %s", modelSlot->name);
      }
    }
  }
}

//TODO(Torin) make these builtin_expected ifs because its incredibly unlikely the asset is not avaiable
//unlikely_if()
ModelAsset* GetModelAsset(U32 modelSlotIndex, AssetManifest* manifest){
  AssetSlot* modelAssetSlot = &manifest->modelAssets[modelSlotIndex];
  if((modelAssetSlot->flags & AssetFlag_Loaded) == 0){
    assert(modelAssetSlot->asset == 0);
    //modelAssetSlot->asset = malloc(modelAssetSlot->requiredMemory); 
    //TODO(Torin) do somthing more along the lines of this
    //HACK: Make this way better

    char modelFilename[1024] = {};
    memcpy(modelFilename, VENOM_ASSET_DIRECTORY, sizeof(VENOM_ASSET_DIRECTORY) -1);
    char *filenameToCheck = modelFilename + sizeof(VENOM_ASSET_DIRECTORY) - 1;
    strcpy(filenameToCheck, modelAssetSlot->filename);

    modelAssetSlot->asset = malloc(sizeof(ModelAsset));
    ModelAsset* modelAsset = (ModelAsset *)modelAssetSlot->asset;
    modelAsset->data = ImportExternalModelData(
      modelFilename, &manifest->memoryBlock);
    modelAsset->aabb = ComputeAABB(&modelAsset->data.meshData);
    modelAsset->size = Abs(modelAsset->aabb.max - modelAsset->aabb.min);
		modelAsset->drawable.materials = ReserveArray(
      MaterialDrawable, modelAsset->data.meshCount, &manifest->memoryBlock);	
    CreateIndexedVertexArray3D(&modelAsset->vertexArray, &modelAsset->data);
		modelAsset->drawable.indexCountPerMesh = modelAsset->data.indexCountPerMesh;
		modelAsset->drawable.vertexArrayID = modelAsset->vertexArray.vertexArrayID;
		modelAsset->drawable.meshCount = modelAsset->data.meshCount;
    modelAsset->lastWriteTime = GetFileLastWriteTime(modelFilename);
		fori(modelAsset->data.meshCount) {
			modelAsset->drawable.materials[i] = 
        CreateMaterialDrawable(&modelAsset->data.materialDataPerMesh[i]);
		}
    modelAsset->name = modelAssetSlot->name;
    modelAssetSlot->flags |= AssetFlag_Loaded;
  }
  ModelAsset *modelAsset = (ModelAsset *)manifest->modelAssets[modelSlotIndex].asset;
  return modelAsset;
}

const ModelDrawable&
GetModelDrawable(U32 modelSlotIndex, AssetManifest* manifest) {
  const ModelAsset* modelAsset = GetModelAsset(modelSlotIndex, manifest);
  return modelAsset->drawable;
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

#if 0
const ModelDrawable&
GetModelDrawable(DEBUGModelID id, GameAssets* assets) {
	DEBUGLoadedModel *loadedModel = &assets->loadedModels[id];
	if(loadedModel->data.meshData.indexCount == 0){
		
    loadedModel->data = ImportExternalModelData(
      MODEL_ASSET_FILENAMES[id], &assets->memory);
    loadedModel->aabb = ComputeAABB(&loadedModel->data.meshData);
		loadedModel->drawable.materials = ReserveArray(
      MaterialDrawable, loadedModel->data.meshCount, &assets->memory);	

		CreateIndexedVertexArray3D(&loadedModel->vertexArray, &loadedModel->data);
		loadedModel->drawable.indexCountPerMesh = loadedModel->data.indexCountPerMesh;
		loadedModel->drawable.vertexArrayID = loadedModel->vertexArray.vertexArrayID;
		loadedModel->drawable.meshCount = loadedModel->data.meshCount;
		fori(loadedModel->data.meshCount) {
			loadedModel->drawable.materials[i] = 
        CreateMaterialDrawable(&loadedModel->data.materialDataPerMesh[i]);
		}
	}
	return loadedModel->drawable;
}
#endif
