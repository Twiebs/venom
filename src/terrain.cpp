
static inline void GenerateTerrainChunk(TerrainGenerationState *terrainGenState, U32 chunkIndexX, U32 chunkIndexZ);
static inline void CreateTerrainBaseMesh(V2 *vertices, U32 *indices);

static inline void CreateTerrainBaseMesh(V2 *vertices, U32 *indices) {
  U32 currentVertexIndex = 0;
  for (U32 z = 0; z < TerrainParameters::CHUNK_SIZE_IN_CELLS + 1; z++) {
    for (U32 x = 0; x < TerrainParameters::CHUNK_SIZE_IN_CELLS + 1; x++) {
      V2 *vertex = &vertices[currentVertexIndex];
      vertex->x = ((float)x * (float)TerrainParameters::CHUNK_SIZE_IN_METERS);
      vertex->y = ((float)z * (float)TerrainParameters::CHUNK_SIZE_IN_METERS);
      currentVertexIndex++;
    }
  }

  U32 currentIndex = 0;
  for (U32 z = 0; z < TerrainParameters::CHUNK_SIZE_IN_CELLS; z++) {
    for (U32 x = 0; x < TerrainParameters::CHUNK_SIZE_IN_CELLS; x++) {
      indices[currentIndex++] = ((z + 0) * (TerrainParameters::CHUNK_SIZE_IN_CELLS + 1)) + (x + 0);
      indices[currentIndex++] = ((z + 1) * (TerrainParameters::CHUNK_SIZE_IN_CELLS + 1)) + (x + 0);
      indices[currentIndex++] = ((z + 1) * (TerrainParameters::CHUNK_SIZE_IN_CELLS + 1)) + (x + 1);
      indices[currentIndex++] = ((z + 0) * (TerrainParameters::CHUNK_SIZE_IN_CELLS + 1)) + (x + 0);
      indices[currentIndex++] = ((z + 1) * (TerrainParameters::CHUNK_SIZE_IN_CELLS + 1)) + (x + 1);
      indices[currentIndex++] = ((z + 0) * (TerrainParameters::CHUNK_SIZE_IN_CELLS + 1)) + (x + 1);
    }
  }
}

//TODO(Torin) Consider making this procedure take a struct to some parameter data
//and actualy return a pointer to the created terrain rather than take it as a paramater
void InitalizeTerrainGenerator(TerrainGenerationState* terrainGenState, V3 centerAt, MemoryBlock* memory) 
{
  //TODO(Torin) The required terrain memory should be known at compile time!
  size_t requiredTerrainMemory = 0;
  requiredTerrainMemory += TerrainParameters::TERRAIN_VERTEX_COUNT; //Heightmap
  requiredTerrainMemory += TerrainParameters::TERRAIN_VERTEX_COUNT; //Detailmap
  requiredTerrainMemory += TerrainParameters::TERRAIN_VERTEX_COUNT * sizeof(V3); //Normals
  
  //TODO(Torin) Change this memory allocation method
  InitSubBlock("TerrainMemory", &terrainGenState->memory, requiredTerrainMemory, memory);
  terrainGenState->heightmap_base = ReserveArray(U8, TerrainParameters::TERRAIN_VERTEX_COUNT, &terrainGenState->memory);
  terrainGenState->detailmap_base = ReserveArray(U8, TerrainParameters::TERRAIN_VERTEX_COUNT, &terrainGenState->memory);
  terrainGenState->normals_base =   ReserveArray(V3, TerrainParameters::TERRAIN_VERTEX_COUNT, &terrainGenState->memory);
  assert(terrainGenState->memory.used == terrainGenState->memory.size);



  for (size_t i = 0; i < TerrainParameters::CHUNK_COUNT; i++) {
    terrainGenState->instanceModelMatrices[i] = M4Identity();
  }

  terrainGenState->heightmap_texture_array = CreateTextureArray(
    TerrainParameters::CHUNK_SIZE_IN_CELLS + 1, TerrainParameters::CHUNK_SIZE_IN_CELLS + 1,
    TerrainParameters::CHUNK_COUNT, GL_R8, GL_REPEAT, GL_LINEAR); 
  terrainGenState->detailmap_texture_array = CreateTextureArray(
    TerrainParameters::CHUNK_SIZE_IN_CELLS + 1, TerrainParameters::CHUNK_SIZE_IN_CELLS + 1,
    TerrainParameters::CHUNK_COUNT, GL_R8, GL_REPEAT, GL_LINEAR);
  terrainGenState->normals_texture_array = CreateTextureArray(
    TerrainParameters::CHUNK_SIZE_IN_CELLS + 1, TerrainParameters::CHUNK_SIZE_IN_CELLS + 1,
    TerrainParameters::CHUNK_COUNT, GL_RGB16F, GL_REPEAT, GL_LINEAR);

  CreateTerrainBaseMesh(terrainGenState->vertices, terrainGenState->indices);
  glGenVertexArrays(1, &terrainGenState->base_mesh.vertexArrayID);
  glBindVertexArray(terrainGenState->base_mesh.vertexArrayID);
  glGenBuffers(1, &terrainGenState->base_mesh.vertexBufferID);
  glBindBuffer(GL_ARRAY_BUFFER, terrainGenState->base_mesh.vertexBufferID);
  glBufferData(GL_ARRAY_BUFFER, TerrainParameters::VERTEX_COUNT_PER_CHUNK * sizeof(V2), terrainGenState->vertices, GL_STATIC_DRAW);
  glGenBuffers(1, &terrainGenState->base_mesh.indexBufferID);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainGenState->base_mesh.indexBufferID);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, TerrainParameters::INDEX_COUNT_PER_CHUNK * sizeof(U32), terrainGenState->indices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(V2), (GLvoid*)0);
  glGenBuffers(1, &terrainGenState->instanceBufferID);
  glBindBuffer(GL_ARRAY_BUFFER, terrainGenState->instanceBufferID);
  glBufferData(GL_ARRAY_BUFFER, TerrainParameters::CHUNK_COUNT * sizeof(M4), terrainGenState->instanceModelMatrices, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glEnableVertexAttribArray(3);
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(M4), (GLvoid*)(sizeof(V4) * 0));
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(M4), (GLvoid*)(sizeof(V4) * 1));
  glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(M4), (GLvoid*)(sizeof(V4) * 2));
  glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(M4), (GLvoid*)(sizeof(V4) * 3));
  glVertexAttribDivisor(1, 1);
  glVertexAttribDivisor(2, 1);
  glVertexAttribDivisor(3, 1);
  glVertexAttribDivisor(4, 1);
  glBindVertexArray(0);

  terrainGenState->lastGenerationTriggerX = centerAt.x;
  terrainGenState->lastGenerationTriggerZ = centerAt.z;
  terrainGenState->currentViewPosition = centerAt;

  terrainGenState->currentOriginInChunkCordsX = S32((centerAt.x / TerrainParameters::CHUNK_SIZE_IN_METERS) - TerrainParameters::TERRAIN_RADIUS);
  terrainGenState->currentOriginInChunkCoordsZ = S32((centerAt.z / TerrainParameters::CHUNK_SIZE_IN_METERS) - TerrainParameters::TERRAIN_RADIUS);

  //TODO(Torin) Should this be done here?
  for (U32 z = 0; z < TerrainParameters::TERRAIN_SIZE_IN_CHUNKS; z++)
    for (U32 x = 0; x < TerrainParameters::TERRAIN_SIZE_IN_CHUNKS; x++)
      GenerateTerrainChunk(terrainGenState, x, z);
}

float GetTerrainHeightAtWorldPosition(TerrainGenerationState *terrainGenState, float x, float z) {
  strict_assert(x >= (terrainGenState->currentOriginInChunkCordsX * TerrainParameters::CHUNK_SIZE_IN_METERS));
  strict_assert(z >= (terrainGenState->currentOriginInChunkCoordsZ * TerrainParameters::CHUNK_SIZE_IN_METERS));
  strict_assert(x <= (terrainGenState->currentOriginInChunkCordsX + TerrainParameters::TERRAIN_SIZE_IN_CHUNKS) * TerrainParameters::CHUNK_SIZE_IN_METERS);
  strict_assert(z <= (terrainGenState->currentOriginInChunkCoordsZ + TerrainParameters::TERRAIN_SIZE_IN_CHUNKS) * TerrainParameters::CHUNK_SIZE_IN_METERS);

  U32 chunkIndexX = (U32)((x / (float)TerrainParameters::CHUNK_SIZE_IN_METERS) - terrainGenState->currentOriginInChunkCordsX);
  U32 chunkIndexZ = (U32)((z / (float)TerrainParameters::CHUNK_SIZE_IN_METERS) - terrainGenState->currentOriginInChunkCoordsZ);
  U32 chunkMemoryIndex = GetTerrainChunkMemoryIndex(terrainGenState,chunkIndexX, chunkIndexZ);
  float worldChunkPosX = ((float)chunkIndexX + terrainGenState->currentOriginInChunkCordsX) * (float)TerrainParameters::CHUNK_SIZE_IN_METERS;
  float worldChunkPosZ = ((float)chunkIndexZ + terrainGenState->currentOriginInChunkCoordsZ) * (float)TerrainParameters::CHUNK_SIZE_IN_METERS;

  float chunkOffsetX = (x - worldChunkPosX);
  float chunkOffsetZ = (z - worldChunkPosZ);

  U32 gridX = (x - worldChunkPosX) / TerrainParameters::CELL_SIZE_METERS;
  U32 gridZ = (z - worldChunkPosZ) / TerrainParameters::CELL_SIZE_METERS;
  U32 gridIndexA = (gridZ * (TerrainParameters::CHUNK_SIZE_IN_CELLS + 1)) + gridX;
  U32 gridIndexB = ((gridZ + 1) * (TerrainParameters::CHUNK_SIZE_IN_CELLS + 1)) + gridX;
  U32 gridIndexC = ((gridZ + 1) * (TerrainParameters::CHUNK_SIZE_IN_CELLS + 1)) + gridX + 1;

  U8 *heightmap = terrainGenState->heightmap_base + (TerrainParameters::VERTEX_COUNT_PER_CHUNK * chunkMemoryIndex);
  float heightA = ((float)heightmap[gridIndexA] / 255.0f) * TERRAIN_HEIGHT_SCALAR;
  float heightB = ((float)heightmap[gridIndexB] / 255.0f) * TERRAIN_HEIGHT_SCALAR;
  float heightC = ((float)heightmap[gridIndexC] / 255.0f) * TERRAIN_HEIGHT_SCALAR;

  V3 p0 = { (float)gridX,   heightA, (float)gridZ };
  V3 p1 = { (float)gridX,   heightB, (float)gridZ+1 };
  V3 p2 = { (float)gridX+1, heightC, (float)gridZ+1 };

  V3 point = ClosestPointOnPlane(V3(x, 0.0f, z), p0, p1, p2);
  return point.y; 
}

//TODO(Torin) Make the terrain chunks consider their neighors when calculating
//the surface normals for each of the edge vertices... We can probably ignore the edge
//cases because the normals will be too far away for it to matter
static void GenerateTerrainChunk(TerrainGenerationState *terrainGenState, U32 chunkIndexX, U32 chunkIndexZ) {
  U32 chunkWriteIndexX = terrainGenState->gpuMemoryOriginX + chunkIndexX;
  U32 chunkWriteIndexZ = terrainGenState->gpuMemoryOriginZ + chunkIndexZ;
  if (chunkWriteIndexX >= TerrainParameters::TERRAIN_SIZE_IN_CHUNKS) 
    chunkWriteIndexX -= TerrainParameters::TERRAIN_SIZE_IN_CHUNKS;
  if (chunkWriteIndexZ >= TerrainParameters::TERRAIN_SIZE_IN_CHUNKS) 
    chunkWriteIndexZ -= TerrainParameters::TERRAIN_SIZE_IN_CHUNKS;
  U32 absoluteChunkMemoryIndex = (chunkWriteIndexZ * TerrainParameters::TERRAIN_SIZE_IN_CHUNKS) + chunkWriteIndexX;
  assert(absoluteChunkMemoryIndex <= TerrainParameters::CHUNK_COUNT);

  S32 chunkCoordX = terrainGenState->currentOriginInChunkCordsX + chunkIndexX;
  S32 chunkCoordZ = terrainGenState->currentOriginInChunkCoordsZ + chunkIndexZ;
  F32 worldCoordX = (F32)chunkCoordX * (F32)TerrainParameters::CHUNK_SIZE_IN_METERS;
  F32 worldCoordZ = (F32)chunkCoordZ * (F32)TerrainParameters::CHUNK_SIZE_IN_METERS;

  static const size_t heightmap_memory_size = (TerrainParameters::VERTEX_COUNT_PER_CHUNK * sizeof(*terrainGenState->heightmap_base));
  static const size_t detailmap_memory_size = (TerrainParameters::VERTEX_COUNT_PER_CHUNK * sizeof(*terrainGenState->detailmap_base));
  static const size_t normalmap_memory_size = (TerrainParameters::VERTEX_COUNT_PER_CHUNK * sizeof(*terrainGenState->normals_base));

  U8* heightmap = terrainGenState->heightmap_base + (absoluteChunkMemoryIndex * heightmap_memory_size);
  U8* detailmap = terrainGenState->detailmap_base + (absoluteChunkMemoryIndex * detailmap_memory_size);
  V3* normals = terrainGenState->normals_base + (absoluteChunkMemoryIndex * TerrainParameters::VERTEX_COUNT_PER_CHUNK);

  //assert((U64)normals < (U64)(terrainGenState->memory.base + terrainGenState->memory.used));
  //assert((U64)(normals + TerrainParameters::VERTEX_COUNT_PER_CHUNK) < (U64)(terrainGenState->memory.base + terrainGenState->memory.used));

  U32 currentVertexIndex = 0;
  for (U32 z = 0; z < TerrainParameters::CHUNK_SIZE_IN_CELLS + 1; z++) {
    for (U32 x = 0; x < TerrainParameters::CHUNK_SIZE_IN_CELLS + 1; x++) {
      float height_scalar = OctaveNoise(worldCoordX + x,  0, worldCoordZ + z, 2, 0.019f, 0.5f); 
      height_scalar = (height_scalar + 1.0f) * 0.5f;
      heightmap[currentVertexIndex] = (U8)(255.0f * height_scalar);
      heightmap[currentVertexIndex] = 0;

      float detail_value = OctaveNoise(worldCoordX + x, 267, worldCoordZ + z, 1, 0.02f, 0.2f);
      detail_value = (detail_value + 1.0f) * 0.5;
      detail_value *= 2.0f;
      detail_value = Clamp01(detail_value);
      detailmap[currentVertexIndex] = (U8)(255.0f * detail_value);
      currentVertexIndex++;
    }
  }

  //TODO(Torin)This thing could just be a lookuptable since the meshdata is baked
  V2* vertices = terrainGenState->vertices;
  U32 *indices = terrainGenState->indices;
  for (U32 i = 0; i < TerrainParameters::INDEX_COUNT_PER_CHUNK; i += 3) {
    U32 index0 = indices[i + 0];
    U32 index1 = indices[i + 1];
    U32 index2 = indices[i + 2];
    assert(index0 <= TerrainParameters::VERTEX_COUNT_PER_CHUNK);
    assert(index1 <= TerrainParameters::VERTEX_COUNT_PER_CHUNK);
    assert(index2 <= TerrainParameters::VERTEX_COUNT_PER_CHUNK);

    V3 v0 = { vertices[index0].x, ((float)heightmap[index0] / 255.0f) * 
      TERRAIN_HEIGHT_SCALAR, vertices[index0].y };
    V3 v1 = { vertices[index1].x, ((float)heightmap[index1] / 255.0f) * 
      TERRAIN_HEIGHT_SCALAR, vertices[index1].y };
    V3 v2 = { vertices[index2].x, ((float)heightmap[index2] / 255.0f) * 
      TERRAIN_HEIGHT_SCALAR, vertices[index2].y };

    V3 edge0 = v1 - v0;
    V3 edge1 = v2 - v0;
    V3 normal = Cross(edge0, edge1);

#if 0
    normals[index0] += normal;
    normals[index1] += normal;
    normals[index2] += normal;
#endif
    normals[index0] = V3(0.0f, 1.0f, 0.0f);
    normals[index1] = V3(0.0f, 1.0f, 0.0f);
    normals[index2] = V3(0.0f, 1.0f, 0.0f);
  }

  for (U32 i = 0; i < TerrainParameters::VERTEX_COUNT_PER_CHUNK; i++) {
    normals[i] = Normalize(normals[i]);
  }

  //NOTE(Torin) The seed cannot be 0 so 1 is added to the chunk memory index
  U64 seed = 0;
  seed |= ((U64)worldCoordX << 0) & 0xFFFFFFFF;
  seed |= ((U64)worldCoordZ << 32) & 0xFFFFFFFF;
  seed += 1;

  auto randf = [&seed]() -> float {
    seed ^= seed >> 12;
    seed ^= seed << 25;
    seed ^= seed >> 27;
    U64 value = seed * (U64)(2685821657736338717);
    float result = (float)((double)value / UINT64_MAX);
    return result;
  };

  U32 map_index = 0;
  for (U32 z = 0; z < TerrainParameters::CHUNK_SIZE_IN_CELLS + 1; z++) {
    for (U32 x = 0; x < TerrainParameters::CHUNK_SIZE_IN_CELLS + 1; x++) {
      if (detailmap[map_index] > 0.5) {

      } 
    } 
    map_index++;
  }


  //TODO(Torin) Move this into a seperate stage after the terrain has 
  //been entirely generated and the normals are being calculated for 
  //the edges of the terrain chunks
  //TODO(Torin 2016-12-22) I think I wrote this here because I wanted to be able
  //to call this asynchronously in the future!

  glBindTexture(GL_TEXTURE_2D_ARRAY, terrainGenState->heightmap_texture_array);
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, absoluteChunkMemoryIndex, 
    (TerrainParameters::CHUNK_SIZE_IN_CELLS + 1), (TerrainParameters::CHUNK_SIZE_IN_CELLS + 1), 1,
    GL_RED, GL_UNSIGNED_BYTE, heightmap);
  glBindTexture(GL_TEXTURE_2D_ARRAY, terrainGenState->normals_texture_array);
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, absoluteChunkMemoryIndex, 
    (TerrainParameters::CHUNK_SIZE_IN_CELLS + 1), (TerrainParameters::CHUNK_SIZE_IN_CELLS + 1), 1,
    GL_RGB, GL_FLOAT, normals);
  glBindTexture(GL_TEXTURE_2D_ARRAY, terrainGenState->detailmap_texture_array);
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, absoluteChunkMemoryIndex, 
    (TerrainParameters::CHUNK_SIZE_IN_CELLS + 1), (TerrainParameters::CHUNK_SIZE_IN_CELLS + 1), 1,
    GL_RED, GL_UNSIGNED_BYTE, detailmap);
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

  terrainGenState->instanceModelMatrices[absoluteChunkMemoryIndex] = Translate(worldCoordX, 0, worldCoordZ);
  glBindBuffer(GL_ARRAY_BUFFER, terrainGenState->instanceBufferID);
  glBufferData(GL_ARRAY_BUFFER, TerrainParameters::CHUNK_COUNT * sizeof(M4), 
   terrainGenState->instanceModelMatrices, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
};

void UpdateTerrainFromViewPosition(TerrainGenerationState *terrain, V3 viewPosition) {
  terrain->currentViewPosition = viewPosition;
  float offsetFromCurrentChunkX = viewPosition.x - terrain->lastGenerationTriggerX;
  float offsetFromCurrentChunkZ = viewPosition.z - terrain->lastGenerationTriggerZ;

  if (offsetFromCurrentChunkX > TerrainParameters::CHUNK_SIZE_IN_METERS || offsetFromCurrentChunkX < (-(S32)TerrainParameters::CHUNK_SIZE_IN_METERS)) {
    terrain->gpuMemoryOriginX += (offsetFromCurrentChunkX > 0.0f ? 1 : -1);
    if (terrain->gpuMemoryOriginX >= (S32)TerrainParameters::TERRAIN_SIZE_IN_CHUNKS)
      terrain->gpuMemoryOriginX = 0;
    if (terrain->gpuMemoryOriginX < 0)
      terrain->gpuMemoryOriginX = TerrainParameters::TERRAIN_SIZE_IN_CHUNKS - 1;
    terrain->lastGenerationTriggerX = viewPosition.x;

    U32 generationChunkIndexX = offsetFromCurrentChunkX > 0 ? (TerrainParameters::TERRAIN_SIZE_IN_CHUNKS - 1) : 0;
    terrain->currentOriginInChunkCordsX += offsetFromCurrentChunkX > 0 ? 1 : -1;
    for (U64 i = 0; i < TerrainParameters::TERRAIN_SIZE_IN_CHUNKS; i++) {
      GenerateTerrainChunk(terrain, generationChunkIndexX, i);
    }
  }

  if (offsetFromCurrentChunkZ > TerrainParameters::CHUNK_SIZE_IN_METERS || offsetFromCurrentChunkZ < -(S32)TerrainParameters::CHUNK_SIZE_IN_METERS) {
    terrain->gpuMemoryOriginZ += (offsetFromCurrentChunkZ > 0.0f ? 1 : -1);
    if (terrain->gpuMemoryOriginZ >= (S32)TerrainParameters::TERRAIN_SIZE_IN_CHUNKS) 
      terrain->gpuMemoryOriginZ = 0;
    if (terrain->gpuMemoryOriginZ < 0) 
      terrain->gpuMemoryOriginZ = TerrainParameters::TERRAIN_SIZE_IN_CHUNKS - 1;
    terrain->lastGenerationTriggerZ = viewPosition.z;

    U32 generationChunkIndexZ = offsetFromCurrentChunkZ > 0 ? (TerrainParameters::TERRAIN_SIZE_IN_CHUNKS - 1) : 0;
    terrain->currentOriginInChunkCoordsZ += offsetFromCurrentChunkZ > 0 ? 1 : -1;
    for (U64 i = 0; i < TerrainParameters::TERRAIN_SIZE_IN_CHUNKS; i++) {
      GenerateTerrainChunk(terrain, i, generationChunkIndexZ);
    }
  }
}