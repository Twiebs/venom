
struct InstancedTerrainParameters {
  U16 terrainRadius;
  U16 chunkWidthInCells;
};

struct InstancedTerrain {
  F32 chunkWidthInMeters;
  
  U16 chunkWidthInCells;
  U16 vertexCountPerChunk;
  U16 cellCountPerChunk;
  U16 chunkCount;
  U16 terrainWidthInChunks;

  GLuint vertexArrayID;
  GLuint vertexBufferID;
};

void InitalizeTerrain(InstancedTerrain *terrain, const InstancedTerrainParameters *params) {
  terrain->chunkWidthInCells = params->chunkWidthInCells;
  terrain->cellCountPerChunk = terrain->chunkWidthInCells * terrain->chunkWidthInCells;
  terrain->vertexCountPerChunk = terrain->cellCountPerChunk * 6;
  terrain->terrainWidthInChunks = params->terrainRadius * 2 + 1;
  terrain->chunkCount = terrain->terrainWidthInChunks * terrain->terrainWidthInChunks;

  glGenVertexArrays(1, &terrain->vertexArrayID);
  glGenBuffers(1, &terrain->vertexBufferID);
  glBindVertexArray(terrain->vertexArrayID);
  glBindBuffer(GL_ARRAY_BUFFER, terrain->vertexBufferID);
  glBufferData(GL_ARRAY_BUFFER, sizeof(V2) * terrain->vertexCountPerChunk, 0, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glEnableVertexAttribArray(3);
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(V2), 0);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(M4), (GLvoid*)(sizeof(V4) * 0));
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(M4), (GLvoid*)(sizeof(V4) * 1));
  glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(M4), (GLvoid*)(sizeof(V4) * 2));
  glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(M4), (GLvoid*)(sizeof(V4) * 3));
  glVertexAttribDivisor(1, 1);
  glVertexAttribDivisor(2, 1);
  glVertexAttribDivisor(3, 1);
  glVertexAttribDivisor(4, 1);

  V2 *vertices = (V2 *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
  for (size_t i = 0; i < terrain->cellCountPerChunk; i++) {
    size_t y = i / terrain->chunkWidthInCells;
    size_t x = i - y;
    vertices[(i * 6) + 0] = V2(x*terrain->chunkWidthInMeters, y*terrain->chunkWidthInMeters);
    vertices[(i * 6) + 1] = V2((x+1)*terrain->chunkWidthInMeters, y*terrain->chunkWidthInMeters);
    vertices[(i * 6) + 2] = V2((x+1)*terrain->chunkWidthInMeters, (y+1)*terrain->chunkWidthInMeters);
    vertices[(i * 6) + 3] = V2(x*terrain->chunkWidthInMeters, y*terrain->chunkWidthInMeters);
    vertices[(i * 6) + 4] = V2((x + 1)*terrain->chunkWidthInMeters, (y + 1)*terrain->chunkWidthInMeters);
    vertices[(i * 6) + 5] = V2((x)*terrain->chunkWidthInMeters, (y + 1)*terrain->chunkWidthInMeters);
  }

  glUnmapBuffer(GL_ARRAY_BUFFER);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}