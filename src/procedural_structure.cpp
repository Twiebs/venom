
struct MeshDrawInfo {
  U32 vertexArrayID;
  U32 materialID;
  U32 indexCount;
  U32 indexOffset;
};

struct Structure {
  IndexedVertexArray vertexArray;
  MeshDrawInfo meshDrawInfos[4];
};

void GenerateStructure(const V3 structureBoundMin, const V3 structureBoundMax, IndexedVertexArray* vertexArray, MeshDrawInfo* drawInfo) {
  Vertex3D vertices[128];
  U32 indices[(ARRAY_COUNT(vertices) / 4) * 6];
  U32 vertexCount = 0, indexCount = 0;

  auto BeginMeshDrawInfo = [](MeshDrawInfo& info, MaterialID id, U32 currentIndexCount) {
    info.materialID = id;
    info.indexOffset = currentIndexCount;
  };

  auto EndMeshDrawInfo = [](MeshDrawInfo& info, U32 currentIndexCount) {
    info.indexCount = currentIndexCount - info.indexOffset;
  };


  const F32 wallSpacing = 0.2f;

  { //Floor
    BeginMeshDrawInfo(drawInfo[0], MaterialID_StoneTile00, indexCount);
    V3 boundsMin = structureBoundMin;
    V3 boundsMax = structureBoundMax;
    GenerateQuadFromPointsCW(
      V3{ boundsMin.x, boundsMin.y, boundsMin.z },
      V3{ boundsMax.x, boundsMin.y, boundsMin.z },
      V3{ boundsMax.x, boundsMin.y, boundsMax.z },
      V3{ boundsMin.x, boundsMin.y, boundsMax.z }, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    EndMeshDrawInfo(drawInfo[0], indexCount);
  }

  BeginMeshDrawInfo(drawInfo[1], MaterialID_WoodFloor00, indexCount);
  { //Exterior Surfaces
    V3 boundsMin = structureBoundMin;
    V3 boundsMax = structureBoundMax;
    GenerateQuadFromPointsCW(
      V3{ boundsMin.x, boundsMin.y, boundsMin.z },
      V3{ boundsMax.x, boundsMin.y, boundsMin.z },
      V3{ boundsMax.x, boundsMax.y, boundsMin.z },
      V3{ boundsMin.x, boundsMax.y, boundsMin.z }, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    GenerateQuadFromPointsCW(
      V3{ boundsMax.x, boundsMin.y, boundsMin.z },
      V3{ boundsMax.x, boundsMin.y, boundsMax.z },
      V3{ boundsMax.x, boundsMax.y, boundsMax.z },
      V3{ boundsMax.x, boundsMax.y, boundsMin.z }, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    GenerateQuadFromPointsCW(
      V3{ boundsMax.x, boundsMin.y, boundsMax.z },
      V3{ boundsMin.x, boundsMin.y, boundsMax.z },
      V3{ boundsMin.x, boundsMax.y, boundsMax.z },
      V3{ boundsMax.x, boundsMax.y, boundsMax.z }, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    GenerateQuadFromPointsCW(
      V3{ boundsMin.x, boundsMin.y, boundsMax.z },
      V3{ boundsMin.x, boundsMin.y, boundsMin.z },
      V3{ boundsMin.x, boundsMax.y, boundsMin.z },
      V3{ boundsMin.x, boundsMax.y, boundsMax.z }, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
  }

  { //Interior Surfaces 
    V3 boundsMin = structureBoundMin +
      V3{ wallSpacing, 0, wallSpacing };
    V3 boundsMax = structureBoundMax -
      V3{ wallSpacing, wallSpacing, wallSpacing };
    GenerateQuadFromPointsCCW(
      V3{ boundsMin.x, boundsMin.y, boundsMin.z },
      V3{ boundsMax.x, boundsMin.y, boundsMin.z },
      V3{ boundsMax.x, boundsMax.y, boundsMin.z },
      V3{ boundsMin.x, boundsMax.y, boundsMin.z }, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    GenerateQuadFromPointsCCW(
      V3{ boundsMax.x, boundsMin.y, boundsMin.z },
      V3{ boundsMax.x, boundsMin.y, boundsMax.z },
      V3{ boundsMax.x, boundsMax.y, boundsMax.z },
      V3{ boundsMax.x, boundsMax.y, boundsMin.z }, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    GenerateQuadFromPointsCCW(
      V3{ boundsMax.x, boundsMin.y, boundsMax.z },
      V3{ boundsMin.x, boundsMin.y, boundsMax.z },
      V3{ boundsMin.x, boundsMax.y, boundsMax.z },
      V3{ boundsMax.x, boundsMax.y, boundsMax.z }, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    GenerateQuadFromPointsCCW(
      V3{ boundsMin.x, boundsMin.y, boundsMax.z },
      V3{ boundsMin.x, boundsMin.y, boundsMin.z },
      V3{ boundsMin.x, boundsMax.y, boundsMin.z },
      V3{ boundsMin.x, boundsMax.y, boundsMax.z }, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    GenerateQuadFromPointsCCW(
      V3{ boundsMin.x, boundsMax.y, boundsMin.z },
      V3{ boundsMax.x, boundsMax.y, boundsMin.z },
      V3{ boundsMax.x, boundsMax.y, boundsMax.z },
      V3{ boundsMin.x, boundsMax.y, boundsMax.z }, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
  }

  EndMeshDrawInfo(drawInfo[1], indexCount);
  CalculateSurfaceNormals(vertices, vertexCount, indices, indexCount);
  CalculateVertexTangents(vertices, indices, vertexCount, indexCount);
  CreateIndexedVertexArray3D(vertexArray, vertices, indices,
    vertexCount, indexCount, GL_STATIC_DRAW);
}

static inline
void generate_structures() {
  //TODO(Torin) Make this paramaterizable by sturucure count
  //instead of split count or better yet have a structure density meteric!
  RNGSeed seed(2);
  U32 splitCount = log2(ARRAY_COUNT(data->structures));
  Rectangle bspTree[ARRAY_COUNT(data->structures)];
  Rectangle bspTreeBounds = { -16.0F, -16.0F, 16.0F, 16.0F };
  U32 currentBSPResultCount = 0;
  GenerateRandomBSPTree(bspTreeBounds, splitCount, seed, bspTree, &currentBSPResultCount);

  for (size_t i = 0; i < ARRAY_COUNT(data->structures); i++) {
    U64 selector = Random01U64(seed);
    Assert(selector <= 1);
    Rectangle bounds = bspTree[(i * 2) + selector];
    F32 width = bounds.maxX - bounds.minX;
    F32 height = bounds.maxY - bounds.minY;
    F32 boundsScalar = RandomInRange(0.6F, 0.8F, seed);
#if 1
    if (width >= height) {
      width *= boundsScalar;
      height *= boundsScalar;
    } else {
      height *= boundsScalar;
      width *= boundsScalar;
    }
#endif

    V3 boundMin = { bounds.minX, 0, bounds.minY };
    V3 boundMax = { bounds.minX + width, 3, bounds.minY + height };

    GenerateStructure(boundMin, boundMax,
      &data->structures[i].vertexArray, &data->structures[i].meshDrawInfos[0]);
    for (size_t n = 0; n < 4; n++) {
      data->structures[i].meshDrawInfos[n].vertexArrayID =
        data->structures[i].vertexArray.vertexArrayID;
    }
  }

}