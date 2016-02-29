
#define TERRAIN_CHUNK_SIZE       31
#define TERRAIN_CELLS_PER_EDGE   31
#define TERRAIN_CHUNK_PER_EDGE   31
#define TERRAIN_ENTITIES_PER_CHUNK 2

static const float TERRAIN_CELL_SIZE = (float)TERRAIN_CHUNK_SIZE / (float)TERRAIN_CELLS_PER_EDGE;
static const U32 TERRAIN_TOTAL_CHUNK_COUNT = TERRAIN_CHUNK_PER_EDGE * TERRAIN_CHUNK_PER_EDGE;
static const U32 TERRAIN_VERTEX_COUNT_PER_CHUNK = (TERRAIN_CELLS_PER_EDGE + 1) * (TERRAIN_CELLS_PER_EDGE + 1);
static const U32 TERRAIN_INDEX_COUNT_PER_CHUNK = (TERRAIN_CELLS_PER_EDGE * TERRAIN_CELLS_PER_EDGE) * 6;

static const U32 TERRAIN_ORIGIN_TO_CENTER_CHUNK_OFFSET = (TERRAIN_CHUNK_PER_EDGE - 1) / 2;

static const U32 TERRAIN_TOTAL_VERTEX_COUNT = TERRAIN_TOTAL_CHUNK_COUNT * TERRAIN_VERTEX_COUNT_PER_CHUNK;
static const U32 TERRAIN_TOTAL_INDEX_COUNT = TERRAIN_TOTAL_CHUNK_COUNT * TERRAIN_INDEX_COUNT_PER_CHUNK;

struct TerrainEntity
{
	V3 position;
	V3 rotation;
	V3 scale;
	const ModelDrawable *drawable;
};

struct TerrainChunk
{
	TerrainEntity *firstGroup;
	TerrainEntity *secondGroup;
	U32 firstCount, secondCount;
};

struct TerrainGenerationParameters
{
	U64 seed;
};

struct TerrainGenerationState
{
	S32 currentOriginX, currentOriginZ;
	S32 gpuMemoryOriginX, gpuMemoryOriginZ;
	float lastGenerationTriggerX, lastGenerationTriggerZ;

	//TerrainChunk chunks[TERRAIN_TOTAL_CHUNK_COUNT];

	//TODO(Torin) is this used?
	U32 currentTerrainMemoryIndex;

	MemoryBlock memory;

	//TODO(Torin) ifdef or remove these in the new terrain system

#if 0
	Vertex3D *verticesBase;
	U32 *indicesBase;
	IndexedVertexArray vertexArray;
#endif

	V2 vertices[TERRAIN_VERTEX_COUNT_PER_CHUNK];
	U32 indices[TERRAIN_INDEX_COUNT_PER_CHUNK];

	IndexedVertexArray base_mesh;
	GLuint instanceBufferID;
	
	GLuint heightmap_texture_array;
	GLuint detailmap_texture_array;
	GLuint normals_texture_array;

	U8 *heightmap_base;
	U8 *detailmap_base;
	V3 *normals_base;
	M4 instanceModelMatrices[TERRAIN_TOTAL_CHUNK_COUNT];

	//TODO(Torin) Somthing smarter with terrain entities
	//or unify the entities into a entity managment system
	//it does not matter how dumb it is for the time being
	//just create something that works
	TerrainEntity *entityBase;
};


static inline U32 GetTerrainChunkMemoryIndex(TerrainGenerationState *terrGenState, U32 chunkIndexX, U32 chunkIndexZ)
{
	U32 chunkWriteIndexX = terrGenState->gpuMemoryOriginX + chunkIndexX;
	U32 chunkWriteIndexZ = terrGenState->gpuMemoryOriginZ + chunkIndexZ;
	if (chunkWriteIndexX >= TERRAIN_CHUNK_PER_EDGE) chunkWriteIndexX -= TERRAIN_CHUNK_PER_EDGE;
	if (chunkWriteIndexZ >= TERRAIN_CHUNK_PER_EDGE) chunkWriteIndexZ -= TERRAIN_CHUNK_PER_EDGE;
	U32 chunkMemoryIndex = (chunkWriteIndexZ * TERRAIN_CHUNK_PER_EDGE) + chunkWriteIndexX;
	return chunkMemoryIndex;
}
