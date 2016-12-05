static void
GenerateQuadFromPointsCCW(
  const V3 a, const V3 b, const V3 c, const V3 d, const F32 materialSizeInMeters,
  Vertex3D* inVertices, U32* inIndices,
  U32* vertexCount, U32* indexCount,
  const U32 maxVertexCount, const U32 maxIndexCount)
{
  assert(*vertexCount + 4 <= maxVertexCount);
  assert(*indexCount + 6 <= maxIndexCount);
  Vertex3D* vertices = inVertices + *vertexCount;
  U32* indices = inIndices + *indexCount;

  V3 displacementX = b - a;
  V3 displacementY = c - b;
  F32 lengthX = Magnitude(displacementX);
  F32 lengthY = Magnitude(displacementY);
  F32 texcoordMaxX = lengthX / materialSizeInMeters;
  F32 texcoordMaxY = lengthY / materialSizeInMeters;

  vertices[0].position = a;
  vertices[1].position = b;
  vertices[2].position = c;
  vertices[3].position = d;
  vertices[0].texcoord = V2{ 0.0f, 0.0f };
  vertices[1].texcoord = V2{ texcoordMaxX, 0.0f };
  vertices[2].texcoord = V2{ texcoordMaxX, texcoordMaxY };
  vertices[3].texcoord = V2{ 0.0f, texcoordMaxY };

  indices[0] = *vertexCount + 0;
  indices[1] = *vertexCount + 1;
  indices[2] = *vertexCount + 2;
  indices[3] = *vertexCount + 0;
  indices[4] = *vertexCount + 2;
  indices[5] = *vertexCount + 3;

  *vertexCount += 4;
  *indexCount += 6;
}

static void
GenerateQuadFromPointsCW(
  const V3 a, const V3 b, const V3 c, const V3 d, const F32 materialSizeInMeters,
  Vertex3D* inVertices, U32* inIndices,
  U32* vertexCount, U32* indexCount,
  const U32 maxVertexCount, const U32 maxIndexCount) {
  GenerateQuadFromPointsCCW(a, d, c, b, materialSizeInMeters,
    inVertices, inIndices, vertexCount, indexCount,
    maxVertexCount, maxIndexCount);
}

void GenerateRandomBSPTree(const Rectangle& root, U32 divisionCount,
  RNGSeed& seed, Rectangle* results, U32* resultsWritten, U32 currentDepth = 0)
{
  static const F32 MINIMUM_SPLIT_PERCENTAGE = 0.4F;
  static const F32 MINIMUM_SPLIT_SIZE = 2.5F;

  S32 splitDirection = Random01U64(seed);
  F32 splitPercentange = RandomInRange(MINIMUM_SPLIT_PERCENTAGE,
    1.0F - MINIMUM_SPLIT_PERCENTAGE, seed);

  Rectangle childA, childB;
  if (splitDirection == 0) { //Vertical Split
    F32 splitSize = (root.maxX - root.minX) * splitPercentange;
    Assert(splitSize > MINIMUM_SPLIT_SIZE);
    childA = { root.minX, root.minY, root.minX + splitSize, root.maxY };
    childB = { childA.maxX, root.minY, root.maxX, root.maxY };
  } else { //Horizontal Split
    F32 splitSize = (root.maxY - root.minY) * splitPercentange;
    Assert(splitSize > MINIMUM_SPLIT_SIZE);
    childA = { root.minX, root.minY, root.maxX, root.minY + splitSize };
    childB = { root.minX, childA.maxY, root.maxX, root.maxY };
  }

  if ((currentDepth + 1) == divisionCount) {
    results[*resultsWritten + 0] = childA;
    results[*resultsWritten + 1] = childB;
    *resultsWritten += 2;
    return;
  }

  GenerateRandomBSPTree(childA, divisionCount, seed,
    results, resultsWritten, currentDepth + 1);
  GenerateRandomBSPTree(childB, divisionCount, seed,
    results, resultsWritten, currentDepth + 1);
}