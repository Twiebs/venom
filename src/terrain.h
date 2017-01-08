
#define TERRAIN_CHUNK_SIZE       31
#define TERRAIN_CELLS_PER_EDGE   31
#define TERRAIN_CHUNK_PER_EDGE   9

static const float TERRAIN_CELL_SIZE = 
  (float)TERRAIN_CHUNK_SIZE / (float)TERRAIN_CELLS_PER_EDGE;

static const U32 TERRAIN_TOTAL_CHUNK_COUNT = 
  TERRAIN_CHUNK_PER_EDGE * TERRAIN_CHUNK_PER_EDGE;

static const U32 TERRAIN_VERTEX_COUNT_PER_CHUNK = 
(TERRAIN_CELLS_PER_EDGE + 1) * (TERRAIN_CELLS_PER_EDGE + 1);
static const U32 TERRAIN_INDEX_COUNT_PER_CHUNK = 
  (TERRAIN_CELLS_PER_EDGE * TERRAIN_CELLS_PER_EDGE) * 6;

static const U32 TERRAIN_ORIGIN_TO_CENTER_CHUNK_OFFSET = (TERRAIN_CHUNK_PER_EDGE - 1) / 2;

static const U32 TERRAIN_TOTAL_VERTEX_COUNT = TERRAIN_TOTAL_CHUNK_COUNT * TERRAIN_VERTEX_COUNT_PER_CHUNK;
static const U32 TERRAIN_TOTAL_INDEX_COUNT = TERRAIN_TOTAL_CHUNK_COUNT * TERRAIN_INDEX_COUNT_PER_CHUNK;

//TODO(Torin) Somthing like this might be much more managable and sane
namespace TerrainParameters {
  static const U32 chunkSize = 31;
  static const U32 cellsPerEdge = 31;
};

struct TerrainGenerationState {
  S32 currentOriginInChunkCordsX;
  S32 currentOriginInChunkCoordsZ;
	S32 gpuMemoryOriginX, gpuMemoryOriginZ;
	float lastGenerationTriggerX, lastGenerationTriggerZ;

  V3 currentViewPosition;

  V2 vertices[TERRAIN_VERTEX_COUNT_PER_CHUNK];
	U32 indices[TERRAIN_INDEX_COUNT_PER_CHUNK];
	M4 instanceModelMatrices[TERRAIN_TOTAL_CHUNK_COUNT];
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

void InitalizeTerrainGenerator(TerrainGenerationState* terrainGenState, MemoryBlock* memory, V3 generationOrigin);
float GetTerrainHeightAtWorldPosition(TerrainGenerationState *terrainGenState, float x, float z);

static inline U32 GetTerrainChunkMemoryIndex(TerrainGenerationState *terrGenState, U32 chunkIndexX, U32 chunkIndexZ) { 
  U32 chunkWriteIndexX = terrGenState->gpuMemoryOriginX + chunkIndexX;
	U32 chunkWriteIndexZ = terrGenState->gpuMemoryOriginZ + chunkIndexZ;
	if (chunkWriteIndexX >= TERRAIN_CHUNK_PER_EDGE)
    chunkWriteIndexX -= TERRAIN_CHUNK_PER_EDGE;
	if (chunkWriteIndexZ >= TERRAIN_CHUNK_PER_EDGE) 
    chunkWriteIndexZ -= TERRAIN_CHUNK_PER_EDGE;
	U32 chunkMemoryIndex = (chunkWriteIndexZ * TERRAIN_CHUNK_PER_EDGE) + chunkWriteIndexX;
	return chunkMemoryIndex;
}
