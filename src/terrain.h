
#if 1
namespace TerrainParameters {
  static const U32 TERRAIN_RADIUS = 2;
  static const U32 CHUNK_SIZE_IN_CELLS = 16;
  static const F32 CHUNK_SIZE_IN_METERS = 16.0f;

  static const U32 TERRAIN_SIZE_IN_CHUNKS = (TERRAIN_RADIUS * 2) + 1;
  static const U32 CHUNK_COUNT = ((TERRAIN_RADIUS * 2 + 1) * (TERRAIN_RADIUS * 2 + 1));
  static const F32 CELL_SIZE_METERS = (F32)CHUNK_SIZE_IN_METERS / (F32)CHUNK_SIZE_IN_CELLS;

  static const U32 VERTEX_COUNT_PER_CHUNK = (CHUNK_SIZE_IN_CELLS + 1) * (CHUNK_SIZE_IN_CELLS + 1);
  static const U32 INDEX_COUNT_PER_CHUNK = (CHUNK_SIZE_IN_CELLS * CHUNK_SIZE_IN_CELLS) * 6;
  static const U32 TERRAIN_VERTEX_COUNT = VERTEX_COUNT_PER_CHUNK * VERTEX_COUNT_PER_CHUNK;
  static const U32 TERRAIN_INDEX_COUNT = INDEX_COUNT_PER_CHUNK * INDEX_COUNT_PER_CHUNK;
};
#endif

struct TerrainGenerationState {
  S32 currentOriginInChunkCordsX;
  S32 currentOriginInChunkCoordsZ;
	S32 gpuMemoryOriginX, gpuMemoryOriginZ;
	float lastGenerationTriggerX, lastGenerationTriggerZ;

  V3 currentViewPosition;

  V2 vertices[TerrainParameters::TERRAIN_VERTEX_COUNT];
	U32 indices[TerrainParameters::TERRAIN_INDEX_COUNT];
	M4 instanceModelMatrices[TerrainParameters::CHUNK_COUNT];
	U8 *heightmap_base;
	U8 *detailmap_base;
	V3 *normals_base;
	MemoryBlock memory;

	IndexedVertexArray base_mesh;
	GLuint instanceBufferID;
	GLuint heightmap_texture_array;
	GLuint detailmap_texture_array;
	GLuint normals_texture_array;
};

void InitalizeTerrainGenerator(TerrainGenerationState* terrainGenState, V3 center, MemoryBlock* memory);
float GetTerrainHeightAtWorldPosition(TerrainGenerationState *terrainGenState, float x, float z);

static inline U32 GetTerrainChunkMemoryIndex(TerrainGenerationState *terrGenState, U32 chunkIndexX, U32 chunkIndexZ) { 
  U32 chunkWriteIndexX = terrGenState->gpuMemoryOriginX + chunkIndexX;
	U32 chunkWriteIndexZ = terrGenState->gpuMemoryOriginZ + chunkIndexZ;
	if (chunkWriteIndexX >= TerrainParameters::TERRAIN_SIZE_IN_CHUNKS)
    chunkWriteIndexX -= TerrainParameters::TERRAIN_SIZE_IN_CHUNKS;
	if (chunkWriteIndexZ >= TerrainParameters::TERRAIN_SIZE_IN_CHUNKS)
    chunkWriteIndexZ -= TerrainParameters::TERRAIN_SIZE_IN_CHUNKS;
	U32 chunkMemoryIndex = (chunkWriteIndexZ * TerrainParameters::TERRAIN_SIZE_IN_CHUNKS) + chunkWriteIndexX;
	return chunkMemoryIndex;
}
