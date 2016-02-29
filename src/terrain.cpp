
#if 0
#include "terrain.h"
static float GetTerrainHeight(TerrainGenerationState& terrainGenState, float x, float z)
{
	U32 chunkIndexX = (U32)((x / (float)TERRAIN_CHUNK_SIZE) - terrainGenState.currentOriginX);
	U32 chunkIndexZ = (U32)((z / (float)TERRAIN_CHUNK_SIZE) - terrainGenState.currentOriginZ);
	U32 chunkMemoryIndex = GetTerrainChunkMemoryIndex(&terrainGenState, chunkIndexX, chunkIndexZ);
	float worldChunkPosX = ((float)chunkIndexX + terrainGenState.currentOriginX) * (float)TERRAIN_CHUNK_SIZE;
	float worldChunkPosZ = ((float)chunkIndexZ + terrainGenState.currentOriginZ) * (float)TERRAIN_CHUNK_SIZE;

	U32 gridX = (x - worldChunkPosX) / TERRAIN_CELL_SIZE;
	U32 gridZ = (z - worldChunkPosZ) / TERRAIN_CELL_SIZE;
	U32 gridIndexA = (gridZ * (TERRAIN_CELLS_PER_EDGE + 1)) + gridX;
	U32 gridIndexB = ((gridZ + 1) * (TERRAIN_CELLS_PER_EDGE + 1)) + gridX;
	U32 gridIndexC = ((gridZ + 1) * (TERRAIN_CELLS_PER_EDGE + 1)) + gridX + 1;

	Vertex3D *vertices = terrainGenState.verticesBase +
		(TERRAIN_VERTEX_COUNT_PER_CHUNK * chunkMemoryIndex);
	Vertex3D& vA = vertices[gridIndexA];
	Vertex3D& vB = vertices[gridIndexB];
	Vertex3D& vC = vertices[gridIndexC];

#if 1
	float gridOffsetX = (x - worldChunkPosX) - ((float)gridX * (float)TERRAIN_CELL_SIZE);
	float gridOffsetZ = (z - worldChunkPosZ) - ((float)gridZ * (float)TERRAIN_CELL_SIZE);
	float heightOffsetZ = vB.position.y - vA.position.y;
	float heightOffsetX = vC.position.y - vA.position.y;
	float heightContribZ = (gridOffsetZ / (float)TERRAIN_CELL_SIZE) * heightOffsetZ;
	float heightContribX = (gridOffsetX / (float)TERRAIN_CELL_SIZE) * heightOffsetX;

	float result = heightContribX + heightContribZ + vA.position.y;
	return result;
#else

	V3 e0 = vB.position - vA.position;
	V3 e1 = vC.position - vA.position;
	V3 normal = Cross(e1, e0);
	float gridOffsetX = (x - worldChunkPosX) - (gridX * TERRAIN_CELL_SIZE);
	float gridOffsetZ = (z - worldChunkPosZ) - (gridZ * TERRAIN_CELL_SIZE);
	float interpX = gridOffsetX / TERRAIN_CELL_SIZE;
	float interpZ = gridOffsetZ / TERRAIN_CELL_SIZE;

	float height0 = Lerp(vA.position.y, vC.position.y, interpZ);
	float height1 = Lerp(vA.position.y, vB.position.y, interpX);
	float height = height0 + height1;
	return height;
#endif
}

static void GenerateTerrain(TerrainGenerationState *terrainGenState, GameAssets *assets, U32 chunkIndexX, U32 chunkIndexZ)
{
	//TODO(Torin) Bake the entire terrain generation grid beside the y height and the normals
	//At compile time and then only sample perlin for the y value and recalculate the normals
	//rather than generating the entire grid for no reason

	//TODO(Torin) The terrain generation system can be switched to work off of a heightmap based system
	//where only a heightmap is requried to be stored in memory and the vertices can just be calculated when nessecary

	U32 chunkWriteIndexX = terrainGenState->gpuMemoryOriginX + chunkIndexX;
	U32 chunkWriteIndexZ = terrainGenState->gpuMemoryOriginZ + chunkIndexZ;
	if (chunkWriteIndexX >= TERRAIN_CHUNK_PER_EDGE) chunkWriteIndexX -= TERRAIN_CHUNK_PER_EDGE;
	if (chunkWriteIndexZ >= TERRAIN_CHUNK_PER_EDGE) chunkWriteIndexZ -= TERRAIN_CHUNK_PER_EDGE;
	U32 absoluteChunkMemoryIndex = (chunkWriteIndexZ * TERRAIN_CHUNK_PER_EDGE) + chunkWriteIndexX;
	size_t vertex_write_offset = sizeof(Vertex3D) * (TERRAIN_VERTEX_COUNT_PER_CHUNK * absoluteChunkMemoryIndex);
	size_t index_write_offset = sizeof(U32) * (TERRAIN_INDEX_COUNT_PER_CHUNK * absoluteChunkMemoryIndex);
	U32 indexCountOffset = absoluteChunkMemoryIndex * TERRAIN_VERTEX_COUNT_PER_CHUNK;

	float worldChunkX = ((float)terrainGenState->currentOriginX * (float)TERRAIN_CHUNK_SIZE) + 
		((float)chunkIndexX * (float)TERRAIN_CHUNK_SIZE);
	float worldChunkZ = ((float)terrainGenState->currentOriginZ * (float)TERRAIN_CHUNK_SIZE) + 
		((float)chunkIndexZ * (float)TERRAIN_CHUNK_SIZE);

	U8 *memory = terrainGenState->memory.base;
	Vertex3D *vertices = (Vertex3D*)(memory + vertex_write_offset);
	U32 *indices = (U32*)(memory + (TERRAIN_VERTEX_COUNT_PER_CHUNK * sizeof(Vertex3D) *
		TERRAIN_TOTAL_CHUNK_COUNT) + index_write_offset);
	U8 *detail_map = terrainGenState->detail_map_base + (TERRAIN_VERTEX_COUNT_PER_CHUNK * absoluteChunkMemoryIndex);

	U32 currentVertexIndex = 0;
	for (U32 z = 0; z < TERRAIN_CELLS_PER_EDGE + 1; z++) {
		for (U32 x = 0; x < TERRAIN_CELLS_PER_EDGE + 1; x++) {
			Vertex3D *vertex = &vertices[currentVertexIndex];
			vertex->position.x =  worldChunkX + ((float)x * (float)TERRAIN_CELL_SIZE);
			vertex->position.z =  worldChunkZ + ((float)z * (float)TERRAIN_CELL_SIZE);
			vertex->position.y = 16 * OctaveNoise(vertex->position.x, 0, vertex->position.z, 8, 0.008f, 0.5f);
			vertex->position.y += 128 * RidgedNoise(vertex->position.x, 0, vertex->position.z, 5, 0.002f, 0.1f);
			vertex->texcoord = ((float)x / 64, (float)z / 64);
			detail_map[currentVertexIndex] = OctaveNoise(vertex->position.x, 1000, vertex->position.z, 4, 0.03f, 0.5f);


			//vertex->position.y = 0.0f;

			//vertex->position.y = ((float)x / (float)TERRAIN_CELLS_PER_EDGE) * 2.0f;
			currentVertexIndex++;
		}
	}

	U32 currentIndex = 0;
	for (U32 z = 0; z < TERRAIN_CELLS_PER_EDGE; z++) {
		for (U32 x = 0; x < TERRAIN_CELLS_PER_EDGE; x++) {
			indices[currentIndex++] = ((z + 0) * (TERRAIN_CELLS_PER_EDGE + 1)) + (x + 0) + indexCountOffset;
			indices[currentIndex++] = ((z + 1) * (TERRAIN_CELLS_PER_EDGE + 1)) + (x + 0) + indexCountOffset;
			indices[currentIndex++] = ((z + 1) * (TERRAIN_CELLS_PER_EDGE + 1)) + (x + 1) + indexCountOffset;
			indices[currentIndex++] = ((z + 0) * (TERRAIN_CELLS_PER_EDGE + 1)) + (x + 0) + indexCountOffset;
			indices[currentIndex++] = ((z + 1) * (TERRAIN_CELLS_PER_EDGE + 1)) + (x + 1) + indexCountOffset;
			indices[currentIndex++] = ((z + 0) * (TERRAIN_CELLS_PER_EDGE + 1)) + (x + 1) + indexCountOffset;
		}
	}

	//NOTE(Torin) The seed cannot be 0 so 1 is added to the chunk memory index
	U64 seed = (U64)(((terrainGenState->currentOriginZ + chunkIndexZ) * TERRAIN_CHUNK_PER_EDGE) + terrainGenState->currentOriginX + chunkIndexX);
	auto randf = [&seed]() -> float
	{
		seed ^= seed >> 12;
		seed ^= seed << 25;
		seed ^= seed >> 27;
		U64 value = seed * (U64)(2685821657736338717);
		float result = (float)((double)value / UINT64_MAX);
		return result;
	};

	TerrainEntity *entityA = &terrainGenState->entityBase[absoluteChunkMemoryIndex * 1];	

	entityA->position.x = worldChunkX + ((float)TERRAIN_CHUNK_SIZE * randf());
	entityA->position.z = worldChunkZ + ((float)TERRAIN_CHUNK_SIZE * randf());
	entityA->position.y = GetTerrainHeight(*terrainGenState, entityA->position.x, entityA->position.z);

	entityA->drawable = &GetModelDrawable(assets, (DEBUGModelID)((U32)((float)DEBUGModelID_COUNT * randf())));
	entityA->scale = V3(1.0f + randf(), 1.0f + randf(), 1.0f + randf());
	entityA->rotation = V3(randf() * (PI32 / 32), randf() * 2 * PI32, randf() * (PI32 / 32));


	//TODO(Torin) Surface normals must be calculated after the terrain has been fullgenerated 
	//NOTE(Torin) takes vertices base because indices need to index correctly into the vertex array
	CalculateSurfaceNormals(terrainGenState->verticesBase, TERRAIN_VERTEX_COUNT_PER_CHUNK,
		(TERRAIN_VERTEX_COUNT_PER_CHUNK * absoluteChunkMemoryIndex), indices, TERRAIN_INDEX_COUNT_PER_CHUNK);

	glBindVertexArray(terrainGenState->vertexArray.vertexArrayID);
	glBindBuffer(GL_ARRAY_BUFFER, terrainGenState->vertexArray.vertexBufferID);
	glBufferSubData(GL_ARRAY_BUFFER, vertex_write_offset, sizeof(Vertex3D) * TERRAIN_VERTEX_COUNT_PER_CHUNK, vertices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainGenState->vertexArray.indexBufferID);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, index_write_offset, sizeof(U32) * TERRAIN_INDEX_COUNT_PER_CHUNK, indices);
	glBindVertexArray(0);
};

#else

static float 
GetTerrainHeightAtWorldPosition(TerrainGenerationState *terrainGenState, float x, float z)
{
	strict_assert(x >= (terrainGenState->currentOriginX * TERRAIN_CHUNK_SIZE));
	strict_assert(z >= (terrainGenState->currentOriginZ * TERRAIN_CHUNK_SIZE));
	strict_assert(x <= (terrainGenState->currentOriginX + TERRAIN_CHUNK_PER_EDGE) * TERRAIN_CHUNK_SIZE);
	strict_assert(z <= (terrainGenState->currentOriginZ + TERRAIN_CHUNK_PER_EDGE) * TERRAIN_CHUNK_SIZE);

	U32 chunkIndexX = (U32)((x / (float)TERRAIN_CHUNK_SIZE) - terrainGenState->currentOriginX);
	U32 chunkIndexZ = (U32)((z / (float)TERRAIN_CHUNK_SIZE) - terrainGenState->currentOriginZ);
	U32 chunkMemoryIndex = GetTerrainChunkMemoryIndex(terrainGenState, chunkIndexX, chunkIndexZ);
	float worldChunkPosX = ((float)chunkIndexX + terrainGenState->currentOriginX) * (float)TERRAIN_CHUNK_SIZE;
	float worldChunkPosZ = ((float)chunkIndexZ + terrainGenState->currentOriginZ) * (float)TERRAIN_CHUNK_SIZE;

	U32 gridX = (x - worldChunkPosX) / TERRAIN_CELL_SIZE;
	U32 gridZ = (z - worldChunkPosZ) / TERRAIN_CELL_SIZE;
	U32 gridIndexA = (gridZ * (TERRAIN_CELLS_PER_EDGE + 1)) + gridX;
	U32 gridIndexB = ((gridZ + 1) * (TERRAIN_CELLS_PER_EDGE + 1)) + gridX;
	U32 gridIndexC = ((gridZ + 1) * (TERRAIN_CELLS_PER_EDGE + 1)) + gridX + 1;

	U8 *heightmap = terrainGenState->heightmap_base + (TERRAIN_VERTEX_COUNT_PER_CHUNK * chunkMemoryIndex);
	U8 heightA = heightmap[gridIndexA];
	U8 heightB = heightmap[gridIndexB];
	U8 heightC = heightmap[gridIndexC];

	float gridOffsetX = (x - worldChunkPosX) - ((float)gridX * (float)TERRAIN_CELL_SIZE);
	float gridOffsetZ = (z - worldChunkPosZ) - ((float)gridZ * (float)TERRAIN_CELL_SIZE);
	float heightOffsetScalarZ = (float)(heightB - heightA) / 255.0f;
	float heightOffsetScalarX = (float)(heightC - heightA) / 255.0f;
	float heightContribZ = (gridOffsetZ / (float)TERRAIN_CELL_SIZE) * heightOffsetScalarZ;
	float heightContribX = (gridOffsetX / (float)TERRAIN_CELL_SIZE) * heightOffsetScalarX;

	float result = heightContribX + heightContribZ + ((float)heightA / 255);
	result *= TERRAIN_HEIGHT_SCALAR;
	return result;
}

static void 
GenerateTerrainChunk(TerrainGenerationState *terrainGenState, GameAssets *assets, U32 chunkIndexX, U32 chunkIndexZ)
{
	U32 chunkWriteIndexX = terrainGenState->gpuMemoryOriginX + chunkIndexX;
	U32 chunkWriteIndexZ = terrainGenState->gpuMemoryOriginZ + chunkIndexZ;
	if (chunkWriteIndexX >= TERRAIN_CHUNK_PER_EDGE) chunkWriteIndexX -= TERRAIN_CHUNK_PER_EDGE;
	if (chunkWriteIndexZ >= TERRAIN_CHUNK_PER_EDGE) chunkWriteIndexZ -= TERRAIN_CHUNK_PER_EDGE;
	U32 absoluteChunkMemoryIndex = (chunkWriteIndexZ * TERRAIN_CHUNK_PER_EDGE) + chunkWriteIndexX;

	float worldChunkX = ((float)terrainGenState->currentOriginX * (float)TERRAIN_CHUNK_SIZE) + 
		((float)chunkIndexX * (float)TERRAIN_CHUNK_SIZE);
	float worldChunkZ = ((float)terrainGenState->currentOriginZ * (float)TERRAIN_CHUNK_SIZE) + 
		((float)chunkIndexZ * (float)TERRAIN_CHUNK_SIZE);

	static const size_t heightmap_memory_size = (TERRAIN_VERTEX_COUNT_PER_CHUNK * 
			sizeof(*terrainGenState->heightmap_base));
	static const size_t detailmap_memory_size = (TERRAIN_VERTEX_COUNT_PER_CHUNK *
			sizeof(*terrainGenState->detailmap_base));
	static const size_t normalmap_memory_size = (TERRAIN_VERTEX_COUNT_PER_CHUNK *
			sizeof(*terrainGenState->normals_base));

	U8* heightmap = terrainGenState->heightmap_base + 
		(absoluteChunkMemoryIndex * heightmap_memory_size);
	U8* detailmap = terrainGenState->detailmap_base + 
		(absoluteChunkMemoryIndex * detailmap_memory_size);
	V3* normals = terrainGenState->normals_base + 
		(absoluteChunkMemoryIndex * normalmap_memory_size);
	TerrainEntity *entities = terrainGenState->entityBase +
		(absoluteChunkMemoryIndex * TERRAIN_ENTITIES_PER_CHUNK);
		
	U32 currentVertexIndex = 0;
	for (U32 z = 0; z < TERRAIN_CELLS_PER_EDGE + 1; z++) 
	{
		for (U32 x = 0; x < TERRAIN_CELLS_PER_EDGE + 1; x++) 
		{
			float height_scalar = OctaveNoise(worldChunkX + x,  0, worldChunkZ + z, 8, 0.008f, 0.5f); 
			//height_scalar += RidgedNoise(worldChunkX + x, 0, worldChunkZ + z, 5, 0.002f, 0.1f);
			height_scalar = (height_scalar + 1.0f) * 0.5f;
			heightmap[currentVertexIndex] = 255 * height_scalar;

			float detail_value = OctaveNoise(worldChunkX + x, 267, worldChunkZ + z, 1, 0.02f, 0.2f);
			detail_value = (detail_value + 1.0f) * 0.5;
			detail_value *= 2.0f;
			detail_value = Clamp01(detail_value);
			detailmap[currentVertexIndex] = (U8)(255.0f * detail_value);

			currentVertexIndex++;
		}
	}

	V2* vertices = terrainGenState->vertices;
	U32 *indices = terrainGenState->indices;
	for (U32 i = 0; i < TERRAIN_INDEX_COUNT_PER_CHUNK; i += 3) 
	{
		auto index0 = indices[i + 0];
		auto index1 = indices[i + 1];
		auto index2 = indices[i + 2];

		V3 v0 = { vertices[index0].x, ((float)heightmap[index0] / 255.0f) * 
			TERRAIN_HEIGHT_SCALAR, vertices[index0].y };
		V3 v1 = { vertices[index1].x, ((float)heightmap[index1] / 255.0f) * 
			TERRAIN_HEIGHT_SCALAR, vertices[index1].y };
		V3 v2 = { vertices[index2].x, ((float)heightmap[index2] / 255.0f) * 
			TERRAIN_HEIGHT_SCALAR, vertices[index2].y };

		auto edge0 = v1 - v0;
		auto edge1 = v2 - v0;
		auto normal = Cross(edge0, edge1);

		normals[index0] += normal;
		normals[index1] += normal;
		normals[index2] += normal;
	}

	for (U32 i = 0; i < TERRAIN_VERTEX_COUNT_PER_CHUNK; i++) 
	{
		normals[i] = Normalize(normals[i]);
	}



	//NOTE(Torin) The seed cannot be 0 so 1 is added to the chunk memory index
	U64 seed = (U64)(((terrainGenState->currentOriginZ + chunkIndexZ) * TERRAIN_CHUNK_PER_EDGE) 
			+ terrainGenState->currentOriginX + chunkIndexX);

	auto randf = [&seed]() -> float
	{
		seed ^= seed >> 12;
		seed ^= seed << 25;
		seed ^= seed >> 27;
		U64 value = seed * (U64)(2685821657736338717);
		float result = (float)((double)value / UINT64_MAX);
		return result;
	};

	U32 map_index = 0;
	for (U32 z = 0; z < TERRAIN_CELLS_PER_EDGE + 1; z++)
	{
		for (U32 x = 0; x < TERRAIN_CELLS_PER_EDGE + 1; x++)
		{
			if (detailmap[map_index] > 0.5)
			{

			}
		}
		map_index++;
	}

#if 1
	for (U32 i = 0; i < TERRAIN_ENTITIES_PER_CHUNK; i++)
	{
		TerrainEntity *entity = &entities[i];
		entity->position.x = worldChunkX + ((float)TERRAIN_CHUNK_SIZE * randf());
		entity->position.z = worldChunkZ + ((float)TERRAIN_CHUNK_SIZE * randf());
		entity->position.y = GetTerrainHeightAtWorldPosition(terrainGenState, entity->position.x, entity->position.z);

		entity->drawable = &GetModelDrawable(assets, (DEBUGModelID)((U32)((float)DEBUGModelID_COUNT * randf())));
		entity->scale = V3(1.0f + randf(), 1.0f + randf(), 1.0f + randf());
		entity->rotation = V3(randf() * (PI32 / 32), randf() * 2 * PI32, randf() * (PI32 / 32));
	}
#endif

	//TODO(Torin) Move this into a seperate stage after the terrain has been entirely generated
	//and the normals are being calculated for the edges of the terrain chunks

	glBindTexture(GL_TEXTURE_2D_ARRAY, terrainGenState->heightmap_texture_array);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, absoluteChunkMemoryIndex, 
			(TERRAIN_CELLS_PER_EDGE + 1), (TERRAIN_CELLS_PER_EDGE + 1), 1,
			GL_RED, GL_UNSIGNED_BYTE, heightmap);
	glBindTexture(GL_TEXTURE_2D_ARRAY, terrainGenState->normals_texture_array);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, absoluteChunkMemoryIndex, 
			(TERRAIN_CELLS_PER_EDGE + 1), (TERRAIN_CELLS_PER_EDGE + 1), 1,
			GL_RGB, GL_FLOAT, normals);
	glBindTexture(GL_TEXTURE_2D_ARRAY, terrainGenState->detailmap_texture_array);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, absoluteChunkMemoryIndex, 
			(TERRAIN_CELLS_PER_EDGE + 1), (TERRAIN_CELLS_PER_EDGE + 1), 1,
			GL_RED, GL_UNSIGNED_BYTE, detailmap);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	terrainGenState->instanceModelMatrices[absoluteChunkMemoryIndex] = Translate(worldChunkX, 0, worldChunkZ);
	glBindBuffer(GL_ARRAY_BUFFER, terrainGenState->instanceBufferID);
	glBufferData(GL_ARRAY_BUFFER, TERRAIN_TOTAL_CHUNK_COUNT * sizeof(M4), 
			terrainGenState->instanceModelMatrices, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
};

#endif
