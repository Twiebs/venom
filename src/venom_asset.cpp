#define VENOM_ASSET_VERIFICATION_NUMBER ('v' << 24) + ('e' << 16) + ('n' << 8) + 'a';
#define VENOM_ASSET_VERSION_NUMBER 1

struct AssetFileHeader {
	U32 verificationNumber;
	U32 versionNumber;
};

void WriteAssetFile(const char *filename)
{
	FILE* file = fopen(filename, "wb");
	if (file == NULL) {
		LOG_ERROR("Could not write asset file");
	}

	AssetFileHeader header = {};
	header.verificationNumber = VENOM_ASSET_VERIFICATION_NUMBER;
	header.versionNumber = VENOM_ASSET_VERSION_NUMBER;
	fwrite(&header, sizeof(AssetFileHeader), 1, file);
	fclose(file);
}

static void HotloadShaders(GameAssets *assets) {
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

GLuint GetShaderProgram(DEBUGShaderID shaderID, GameAssets *assets) {
	DEBUGLoadedShader *loadedShader = &assets->loadedShaders[shaderID];
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

const ModelDrawable&
GetModelDrawable(DEBUGModelID id, GameAssets* assets) {
	DEBUGLoadedModel *loadedModel = &assets->loadedModels[id];
	if (loadedModel->data.meshData.indexCount == 0) {
		loadedModel->data = ImportExternalModelData(
      DEBUG_MODEL_INFOS[id].filename, &assets->memory);
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

const MaterialDrawable& 
GetMaterial(U32 id, GameAssets* assets) {
  MaterialAssetList* list = &assets->materialAssetList;
  MaterialAsset* materialAsset = &list->materials[id];
  assert(id < list->materialCount);
	if (materialAsset->drawable.diffuse_texture_id == 0) {
		DebugMaterialInfo info = {};
		info.filenames[MaterialTextureType_DIFFUSE]  = materialAsset->filenames[0];
		info.filenames[MaterialTextureType_NORMAL]   = materialAsset->filenames[1];
		info.filenames[MaterialTextureType_SPECULAR] = materialAsset->filenames[2];
		materialAsset->data.flags = materialAsset->flags;
		CreateMaterialData(&materialAsset->data, &info, &assets->memory);
		materialAsset->drawable = CreateMaterialDrawable(&materialAsset->data);
	}
	return materialAsset->drawable;
}
