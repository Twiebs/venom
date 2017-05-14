
struct TerrainVertex {
  V3 position;
  V3 normal;
  V3 color;
};

struct TerrainChunk {
  GLuint vertexArrayID;
  GLuint vertexBufferID;
  DynamicArray<EntityIndex> entities;
};

struct Terrain {
  TerrainChunk *chunks;
  size_t chunkCount;

  //Configuration Values
  U16 terrainRadius;
  U16 chunkWidthInCells;
  F32 chunkWidthInMeters;
  //Calculated Values
  U16 terrainWidthInChunks;
  U16 vertexCountPerChunk;
  U16 cellCountPerChunk;
  F32 cellWidthInMeters;
};

F32 GetTerrainHeightAtPosition(Terrain *terrain, V3 position);
void InitalizeTerrain(Terrain *terrain, U32 terrainRadius, U32 chunkWidthInCells, F32 chunkWidthInMeters);