
F32 TerrainHeightFunction(F32 x, F32 z) {
  F32 result = OctaveNoise(x, 0, z, 8, 0.009f, 0.5f);
  result *= 15.0;
  return result;
}

void GenerateTerrainChunk(Terrain *terrain, TerrainChunk *chunk, F32 originX, F32 originZ) {
  glBindBuffer(GL_ARRAY_BUFFER, chunk->vertexBufferID);
  TerrainVertex *vertices = (TerrainVertex *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

  V3 color = V3(0.2f, 0.8f, 0.3f);
  for (size_t i = 0; i < terrain->cellCountPerChunk; i++) {
    size_t z = i / terrain->chunkWidthInCells;
    size_t x = i % terrain->chunkWidthInCells;

    { //Top Triangle
      F32 aHeight = TerrainHeightFunction(((x + 0) * terrain->cellWidthInMeters + originX), ((z + 1) * terrain->cellWidthInMeters + originZ));
      F32 bHeight = TerrainHeightFunction(((x + 1) * terrain->cellWidthInMeters + originX), ((z + 0) * terrain->cellWidthInMeters + originZ));
      F32 cHeight = TerrainHeightFunction(((x + 0) * terrain->cellWidthInMeters + originX), ((z + 0) * terrain->cellWidthInMeters + originZ));
      V3 a = V3(((x + 0) * terrain->cellWidthInMeters) + originX, aHeight, ((z + 1) * terrain->cellWidthInMeters) + originZ);
      V3 b = V3(((x + 1) * terrain->cellWidthInMeters) + originX, bHeight, ((z + 0) * terrain->cellWidthInMeters) + originZ);
      V3 c = V3(((x + 0) * terrain->cellWidthInMeters) + originX, cHeight, ((z + 0) * terrain->cellWidthInMeters) + originZ);
      V3 normal = Normalize(Cross(b - a, c - a));
      vertices[(i * 6) + 0] = { a, normal, color };
      vertices[(i * 6) + 1] = { b, normal, color };
      vertices[(i * 6) + 2] = { c, normal, color };
    }


    { //Bottom triangle
      F32 aHeight = TerrainHeightFunction(((x + 0) * terrain->cellWidthInMeters) + originX, ((z + 1) * terrain->cellWidthInMeters + originZ));
      F32 bHeight = TerrainHeightFunction(((x + 1) * terrain->cellWidthInMeters) + originX, ((z + 1) * terrain->cellWidthInMeters + originZ));
      F32 cHeight = TerrainHeightFunction(((x + 1) * terrain->cellWidthInMeters) + originX, ((z + 0) * terrain->cellWidthInMeters + originZ));
      V3 a = V3(((x + 0) * terrain->cellWidthInMeters) + originX, aHeight, ((z + 1) * terrain->cellWidthInMeters) + originZ);
      V3 b = V3(((x + 1) * terrain->cellWidthInMeters) + originX, bHeight, ((z + 1) * terrain->cellWidthInMeters) + originZ);
      V3 c = V3(((x + 1) * terrain->cellWidthInMeters) + originX, cHeight, ((z + 0) * terrain->cellWidthInMeters) + originZ);
      V3 normal = Normalize(Cross(b - a, c - a));
      vertices[(i * 6) + 3] = { a, normal, color };
      vertices[(i * 6) + 4] = { b, normal, color }; 
      vertices[(i * 6) + 5] = { c, normal, color };
    }


  }

  glUnmapBuffer(GL_ARRAY_BUFFER);
}

F32 GetTerrainHeightAtPosition(Terrain *terrain, V3 position) {
  S32 chunkXIndex = position.x / terrain->chunkWidthInMeters;
  S32 chunkZIndex = position.z / terrain->chunkWidthInMeters;
  if (chunkXIndex < 0 || chunkXIndex >= terrain->terrainWidthInChunks) return 0.0f;
  if (chunkZIndex < 0 || chunkZIndex >= terrain->terrainWidthInChunks) return 0.0f;

  F32 localXCoord = position.x - (chunkXIndex * terrain->chunkWidthInMeters);
  F32 localZCoord = position.z - (chunkZIndex * terrain->chunkWidthInMeters);
  S32 cellXIndex = (S32)(localXCoord / terrain->cellWidthInMeters);
  S32 cellZIndex = (S32)(localZCoord / terrain->cellWidthInMeters);
  F32 cellLocalX = fmod(localXCoord, terrain->cellWidthInMeters) / terrain->cellWidthInMeters;
  F32 cellLocalZ = fmod(localZCoord, terrain->cellWidthInMeters) / terrain->cellWidthInMeters;

  U64 chunkIndex = (chunkZIndex * terrain->terrainWidthInChunks) + chunkXIndex;
  TerrainChunk *chunk = &terrain->chunks[chunkIndex];
  glBindBuffer(GL_ARRAY_BUFFER, chunk->vertexBufferID);
  TerrainVertex *vertices = (TerrainVertex *)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
  uintptr_t cellOffset = (((S32)cellZIndex * terrain->chunkWidthInCells) + (S32)cellXIndex);
  TerrainVertex *triangle = vertices + cellOffset;
  
  if (cellLocalX >= (1.0f - cellLocalZ)) { //Inside bottom triangle
    triangle += 3;
  }

  V2 p = V2(position.x, position.z);
  V2 a = V2(triangle[0].position.x, triangle[0].position.z);
  V2 b = V2(triangle[1].position.x, triangle[1].position.z);
  V2 c = V2(triangle[2].position.x, triangle[2].position.z);
  F32 abcArea = Orient2D(a, b, c) / 2;

  F32 u = (Orient2D(a, b, p) / 2) / abcArea;
  F32 v = (Orient2D(b, c, p) / 2) / abcArea;
  F32 w = 1.0 - u - v;
  assert(Equals(u + v + w, 1.0f));
  F32 result = triangle[0].position.y*v + triangle[1].position.y*w + triangle[2].position.y*u;

  glUnmapBuffer(GL_ARRAY_BUFFER);
  return result;
}

void InitalizeTerrain(Terrain *terrain, U32 terrainRadius, U32 chunkWidthInCells, F32 chunkWidthInMeters) {
  terrain->terrainRadius = terrainRadius;
  terrain->chunkWidthInCells = chunkWidthInCells;
  terrain->chunkWidthInMeters = chunkWidthInMeters;
  terrain->terrainWidthInChunks = 2 * terrain->terrainRadius + 1;
  terrain->chunkCount = terrain->terrainWidthInChunks * terrain->terrainWidthInChunks;
  terrain->cellCountPerChunk = terrain->chunkWidthInCells * terrain->chunkWidthInCells;
  terrain->vertexCountPerChunk = terrain->cellCountPerChunk * 6;
  terrain->cellWidthInMeters = terrain->chunkWidthInMeters / (F32)terrain->chunkWidthInCells;

  size_t requiredMemory = terrain->chunkCount * sizeof(TerrainChunk);
  U8 *memory = MemoryAllocate(requiredMemory);
  terrain->chunks = (TerrainChunk *)memory;

  for (size_t i = 0; i < terrain->chunkCount; i++) {
    TerrainChunk *chunk = &terrain->chunks[i];
    glGenVertexArrays(1, &chunk->vertexArrayID);
    glGenBuffers(1, &chunk->vertexBufferID);
    glBindVertexArray(chunk->vertexArrayID);
    glBindBuffer(GL_ARRAY_BUFFER, chunk->vertexBufferID);
    glBufferData(GL_ARRAY_BUFFER, terrain->vertexCountPerChunk * sizeof(TerrainVertex), 0, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void *)offsetof(TerrainVertex, position));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void *)offsetof(TerrainVertex, normal));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void *)offsetof(TerrainVertex, color));
  }

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  size_t index = 0;
  for (size_t z = 0; z < terrain->terrainWidthInChunks; z++) {
    for (size_t x = 0; x < terrain->terrainWidthInChunks; x++) {
      TerrainChunk *chunk = &terrain->chunks[index++];
      GenerateTerrainChunk(terrain, chunk, x * terrain->chunkWidthInMeters, z * terrain->chunkWidthInMeters);
    }
  }
}

