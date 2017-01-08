struct RNGSeed {
	U64 seed;
  RNGSeed(U64 seed) : seed(seed) { assert(seed != 0); }
};

U64 RandomU64(RNGSeed& seed) {
  assert(seed.seed != 0);
  seed.seed ^= seed.seed >> 12;
  seed.seed ^= seed.seed << 25;
  seed.seed ^= seed.seed >> 27;
  U64 value = seed.seed * (U64)(2685821657736338717);
  return value;
};

F64 RandomF01(RNGSeed& seed) {
  U64 value = RandomU64(seed);
  F64 result = ((F64)value / UINT64_MAX);
  return result;
}

F64 RandomInRange(F32 min, F32 max, RNGSeed& seed) {
  F64 result = (RandomF01(seed) * (max - min)) + min;
  return result;
}

U64 Random01U64 (RNGSeed& seed) {
  U64 result = RandomU64(seed) / (UINT64_MAX / 2);
  return result; 
}

inline V4 RandomSolidColor(RNGSeed& seed) {
	V4 result;
	result.x = RandomInRange(0.0f, 1.0f, seed);
	result.y = RandomInRange(0.0f, 1.0f, seed);
	result.z = RandomInRange(0.0f, 1.0f, seed);
	result.w = 1.0f;
	return result;
}

inline V3 RandomInRange(const V3& min, const V3& max, RNGSeed& seed) {
	V3 result;
	result.x = RandomInRange(min.x, max.x, seed);
	result.y = RandomInRange(min.y, max.y, seed);
	result.z = RandomInRange(min.z, max.z, seed);
	return result;
}


void GetSubdiviedCubeVertexAndIndexCount( uint32_t cellsPerEdge, 
	uint32_t *vertexCount, uint32_t *indexCount);
void GenerateSubdiviedCubeMeshData(U32 cellsPerEdge, V3 *vertices,  U32 *indices);
