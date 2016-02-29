
void InitalizeGameAssets(GameAssets *assets)
{
	U32 i = 0;

#define _(name, diffuse, normal, specular, flags) assets->materials[i] = { #name, {VENOM_ASSET_FILE(diffuse), VENOM_ASSET_FILE(normal), VENOM_ASSET_FILE(specular)}, flags, {}, {}}; i++;
		DebugMaterialList
#undef _

}


#define VENOM_ASSET_VERIFICATION_NUMBER ('v' << 24) + ('e' << 16) + ('n' << 8) + 'a';
#define VENOM_ASSET_VERSION_NUMBER 1
struct AssetFileHeader
{
	U32 verificationNumber;
	U32 versionNumber;
};

void WriteAssetFile(const char *filename)
{
	FILE* file = fopen(filename, "wb");
	if (file == NULL)
	{
		LOG_ERROR("Could not write asset file");
	}

	AssetFileHeader header = {};
	header.verificationNumber = VENOM_ASSET_VERIFICATION_NUMBER;
	header.versionNumber = VENOM_ASSET_VERSION_NUMBER;
	fwrite(&header, sizeof(AssetFileHeader), 1, file);
	fclose(file);
}

static void HotloadShaders(GameAssets *assets)
{
	for (U32 i = 0; i < DEBUGShaderID_COUNT; i++)
	{
		DEBUGLoadedShader *loadedShader = assets->loadedShaders + i;
		if (!loadedShader->is_loaded) continue;

		for (auto n = 0; n < 4; n++)
		{
			const char *filename = loadedShader->filenames[n];
			if (filename != nullptr)
			{
				U64 lastWriteTime = GetFileLastWriteTime(filename);
				if (lastWriteTime != loadedShader->lastWriteTimes[n])
				{
					loadedShader->lastWriteTimes[0] = GetFileLastWriteTime(loadedShader->filenames[0]);
					loadedShader->lastWriteTimes[1] = GetFileLastWriteTime(loadedShader->filenames[1]);
					loadedShader->lastWriteTimes[2] = GetFileLastWriteTime(loadedShader->filenames[2]);
					GLuint newShader = DEBUGCreateShaderProgramFromFiles(loadedShader->filenames);
					if (newShader != 0)
					{
						glDeleteProgram(loadedShader->programHandle);		
						loadedShader->programHandle = DEBUGCreateShaderProgramFromFiles(loadedShader->filenames);
						glDeleteProgram(newShader);
						LOG_DEBUG("Reloaded shader program");
					}
					break;
				}	
			}
		}
	}
}

GLuint GetShaderProgram(GameAssets *assets, DEBUGShaderID shaderID) 
{
	DEBUGLoadedShader *loadedShader = &assets->loadedShaders[shaderID];
	if (loadedShader->programHandle == 0)
	{
		for (int i = 0; i < ShaderType_COUNT; i++)
		{
			loadedShader->filenames[i] = DEBUG_SHADER_INFOS[shaderID].filenames[i];
			loadedShader->lastWriteTimes[i] = GetFileLastWriteTime(loadedShader->filenames[i]);
		}

		loadedShader->programHandle = DEBUGCreateShaderProgramFromFiles(loadedShader->filenames);
		loadedShader->is_loaded = true;
	}
	
	return loadedShader->programHandle;
}

MaterialDrawable CreateDrawableMaterial(MaterialData *data)
{
	MaterialDrawable drawable = {};
	drawable.flags = data->flags;
	U8 *pixels_to_read = data->textureData;
	GLenum wrap_mode = GL_CLAMP_TO_EDGE;
	if (data->flags & MaterialFlag_REPEAT) wrap_mode = GL_REPEAT;

	if (data->flags & MaterialFlag_TRANSPARENT)
	{


		drawable.diffuse_texture_id = CreateTextureWithMipmaps(pixels_to_read, data->textureWidth, data->textureHeight, GL_RGBA8, GL_RGBA, wrap_mode);
		pixels_to_read += (data->textureWidth * data->textureHeight * 4);
	} else
	{
		drawable.diffuse_texture_id = CreateTextureWithMipmaps(pixels_to_read, data->textureWidth, data->textureHeight, GL_RGB8, GL_RGB, wrap_mode);
		pixels_to_read += (data->textureWidth * data->textureHeight * 3);
	}

	if (data->flags & MaterialFlag_NORMAL)
	{
		drawable.normal_texture_id = CreateTextureWithMipmaps(pixels_to_read, data->textureHeight, data->textureHeight, GL_RGB8, GL_RGB, wrap_mode);
		pixels_to_read += (data->textureWidth * data->textureHeight * 3);
	}
	if (data->flags & MaterialFlag_SPECULAR)
	{
		drawable.specular_texture_id = CreateTextureWithMipmaps(pixels_to_read, data->textureHeight, data->textureHeight, GL_RGB8, GL_RGB, wrap_mode);
	}

	return drawable;
}

void DEBUGLoadModel(DEBUGLoadedModel* loadedModel, const char *filename, MemoryBlock *memblock)
{
	loadedModel->data = ImportExternalModelData(filename, memblock);
	loadedModel->drawable.materials = ReserveArray(MaterialDrawable, loadedModel->data.meshCount, memblock);
	CreateIndexedVertexArray3D(&loadedModel->vertexArray, &loadedModel->data);
	loadedModel->drawable.indexCountPerMesh = loadedModel->data.indexCountPerMesh;
	loadedModel->drawable.vertexArrayID = loadedModel->vertexArray.vertexArrayID;
	loadedModel->drawable.meshCount = loadedModel->data.meshCount;
	fori(loadedModel->data.meshCount)
	{
		loadedModel->drawable.materials[i] = CreateDrawableMaterial(&loadedModel->data.materialDataPerMesh[i]);
	}
}

const ModelDrawable &GetModelDrawable(GameAssets *assets, DEBUGModelID id)
{
	DEBUGLoadedModel *loadedModel = &assets->loadedModels[id];
	if (loadedModel->data.meshData.indexCount == 0)
	{
		loadedModel->data = ImportExternalModelData(DEBUG_MODEL_INFOS[id].filename, &assets->memory);
		loadedModel->drawable.materials = ReserveArray(MaterialDrawable, loadedModel->data.meshCount, &assets->memory);	
		CreateIndexedVertexArray3D(&loadedModel->vertexArray, &loadedModel->data);
		loadedModel->drawable.indexCountPerMesh = loadedModel->data.indexCountPerMesh;
		loadedModel->drawable.vertexArrayID = loadedModel->vertexArray.vertexArrayID;
		loadedModel->drawable.meshCount = loadedModel->data.meshCount;
		fori(loadedModel->data.meshCount)
		{
			loadedModel->drawable.materials[i] = CreateDrawableMaterial(&loadedModel->data.materialDataPerMesh[i]);
		}
	}

	return loadedModel->drawable;
}

const MaterialDrawable& GetMaterial(GameAssets *assets, DEBUGMaterialID id)
{
	DEBUGMaterialAsset *material = &assets->materials[id];
	if (material->drawable.diffuse_texture_id == 0)
	{
		DebugMaterialInfo info = {};
		info.diffuse_filename = material->filenames[0];
		info.normal_filename = material->filenames[1];
		info.specular_filename = material->filenames[2];
		material->data.flags = material->flags;
		CreateMaterialData(&material->data, &info, &assets->memory);
		material->drawable = CreateDrawableMaterial(&material->data);
	}
	return material->drawable;
}
