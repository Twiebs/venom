#define VENOM_MATERIAL_LIST_FILE "asset_list.h"

#include "venom_platform.h"
#include "venom_module.cpp"

#include "math_procedural.cpp"

#define EntityTypeList \
  _(Container) \
  _(PointLight) \

enum EntityType {
#define _(name) EntityType_##name,
  EntityTypeList
  EntityType_Count
#undef _
};

static const char* ENTITY_TYPE_STRINGS[] = {
#define _(name) #name,
  EntityTypeList
#undef _
};

enum EntityFlag {
  EntityFlag_ACTIVE = 1 << 0,
};

struct PointLightEntity {
  V3 color;
  F32 radius;
};

struct Bullet {
  F32 decayTime;
  V3 velocity; 
};

struct Entity {
  EntityType type;
  V3 position;
  U32 modelID;

  union {
    PointLightEntity pointLight;
  };
};

#define ENTITY_CAPACITY 128
struct EntityArray {
  U64 flags[ENTITY_CAPACITY];
  Entity entities[ENTITY_CAPACITY];
};

U64 CreateEntity(EntityType type, EntityArray* array) {
  U64 entityID = (1L << 32); 
  for (U64 i = 0; i < ENTITY_CAPACITY; i++) {
    if ((array->flags[i] & EntityFlag_ACTIVE) == 0) {
      entityID = i;
      break;
    }
  }

  if (entityID != (1L << 32)) {
    array->flags[entityID] |= EntityFlag_ACTIVE;
    Entity& entity = array->entities[entityID];
    entity.type = type;
  }

  return entityID;
}

struct MeshDrawInfo {
  U32 vertexArrayID;
  U32 materialID;
  U32 indexCount;
  U32 indexOffset;
};

struct Player {
  V3 position;
  V3 velocity;
};

struct GameData {
  Camera camera;
  IndexedVertexArray proceduralWorld;
  V3 testPositions[100];
  MeshDrawInfo meshDrawInfos[4];
  V3 containerPositions[4];
  EntityArray entityArray;
  bool initalized;
  Player player;
};


static void 
GenerateQuadFromPointsCCW(
  const V3 a, const V3 b, const V3 c, const V3 d, const F32 materialSizeInMeters,
  Vertex3D* inVertices, U32* inIndices,
  U32* vertexCount, U32* indexCount, 
  const U32 maxVertexCount, const U32 maxIndexCount)
{
  assert(*vertexCount + 4 < maxVertexCount);
  assert(*indexCount + 6 < maxIndexCount);
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
  vertices[0].texcoord = V2 { 0.0f, 0.0f };
  vertices[1].texcoord = V2 { texcoordMaxX, 0.0f };
  vertices[2].texcoord = V2 { texcoordMaxX, texcoordMaxY};
  vertices[3].texcoord = V2 { 0.0f, texcoordMaxY};
  
  indices[0] = *vertexCount + 0;
  indices[1] = *vertexCount + 1;
  indices[2] = *vertexCount + 2;
  indices[3] = *vertexCount + 0;
  indices[4] = *vertexCount + 2;
  indices[5] = *vertexCount + 3;
  
  *vertexCount+= 4;
  *indexCount+= 6;
}


static void 
GenerateQuadFromPointsCW(
  const V3 a, const V3 b, const V3 c, const V3 d, const F32 materialSizeInMeters,
  Vertex3D* inVertices, U32* inIndices,
  U32* vertexCount, U32* indexCount, 
  const U32 maxVertexCount, const U32 maxIndexCount)
{
  GenerateQuadFromPointsCCW(a, d, c, b, materialSizeInMeters,
    inVertices, inIndices, vertexCount, indexCount,
    maxVertexCount, maxIndexCount);
}




static void 
GenerateQuadByExtrudingLineSegment(const V3 start, const V3 end, const F32 lengthY, 
  Vertex3D* vertices, U32* indices,
  U32* vertexCount, U32* indexCount, 
  const U32 maxVertexCount, const U32 maxIndexCount) 
{
  assert(*vertexCount + 4 < maxVertexCount);
  assert(*indexCount + 6 < maxIndexCount);
  vertices += *vertexCount;
  indices += *indexCount;

  F32 lengthX = std::abs(end.x - start.x);
  F32 lengthZ = std::abs(end.z - start.z);

  vertices[0].position = start;
  vertices[1].position = end;
  vertices[2].position = end + V3 { 0, lengthY, 0 };
  vertices[3].position = start + V3 { 0, lengthY, 0 }; 

  vertices[0].texcoord = V2 { 0.0f, 0.0f };
  vertices[1].texcoord = V2 { 4.0f, 0.0f };
  vertices[2].texcoord = V2 { 4.0f, 2.0f};
  vertices[3].texcoord = V2 { 0.0f, 2.0f};

  indices[0] = *vertexCount + 0;
  indices[1] = *vertexCount + 1;
  indices[2] = *vertexCount + 2;
  indices[3] = *vertexCount + 0;
  indices[4] = *vertexCount + 2;
  indices[5] = *vertexCount + 3;
  *vertexCount+= 4;
  *indexCount+= 6;
}

void GenerateQuad(const V3 position, const V2 size, 
  const F32 materialSizeInMeters, const bool faceDown,
  Vertex3D* vertices, U32* indices,
  U32* vertexCount, U32* indexCount,
  const U32 maxVertexCount, const U32 maxIndexCount) 
{
  assert(*vertexCount + 4 <= maxVertexCount);
  assert(*indexCount + 6 <= maxIndexCount);
  vertices += *vertexCount;
  indices += *indexCount;

  F32 aspectRatio = size.y / size.x;
  F32 textureRepeatCount = size.x / materialSizeInMeters; 
  
  vertices[0].position = position;
  vertices[1].position = position + V3{ size.x, 0, 0 };
  vertices[2].position = position + V3{ size.x, 0, size.y };
  vertices[3].position = position + V3{ 0, 0, size.y };
  vertices[0].texcoord = V2 { 0.0f, 0.0f };
  vertices[1].texcoord = V2 { textureRepeatCount, 0.0f };
  vertices[2].texcoord = V2 { textureRepeatCount, textureRepeatCount * aspectRatio};
  vertices[3].texcoord = V2 { 0.0f, textureRepeatCount * aspectRatio};

  if (faceDown) {
    indices[0] = *vertexCount + 0;
    indices[1] = *vertexCount + 1;
    indices[2] = *vertexCount + 2;
    indices[3] = *vertexCount + 0;
    indices[4] = *vertexCount + 2;
    indices[5] = *vertexCount + 3;
  } else {
    indices[0] = *vertexCount + 0;
    indices[1] = *vertexCount + 3;
    indices[2] = *vertexCount + 2;
    indices[3] = *vertexCount + 0;
    indices[4] = *vertexCount + 2;
    indices[5] = *vertexCount + 1;
  }

  *vertexCount += 4;
  *indexCount += 6;
}

void VenomModuleStart(GameMemory* memory) {
  SystemInfo* sys = &memory->systemInfo;
  GameData* data = PushStruct(GameData, &memory->mainBlock);
  RenderState* rs = &memory->renderState;


  memory->userdata = data;
  InitGBuffer(&rs->gbuffer, sys->screen_width, sys->screen_height);
  InitCascadedShadowMaps(&memory->renderState.csm, 
    sys->screen_width, sys->screen_height, memory->renderState.debugCamera.fov);
  
  for(size_t i = 0; i < 4; i++)
    InitOmnidirectionalShadowMap(&rs->osm[i]);

  InitializeCamera(&data->camera, 45*DEG2RAD, 0.1f, 100.0f,
    sys->screen_width, sys->screen_height);
  data->camera.position = {4, 10, 2};
  data->camera.pitch = -89.99f*DEG2RAD;
  data->camera.yaw = -90.0f*DEG2RAD;

  Vertex3D vertices[128];
  U32 indices[(ARRAY_COUNT(vertices) / 4) * 6];
  U32 vertexCount = 0, indexCount = 0;

  V3 structureOrigin = { 0, 0.01F, 0 };
  V3 structureSize = { 8, 2.6F, 5 };
 
  data->meshDrawInfos[0].materialID = MaterialID_dirt00;
  data->meshDrawInfos[0].indexCount = 6;
  data->meshDrawInfos[0].indexOffset = indexCount;
  GenerateQuad(V3 { -32, 0, -32}, V2 {64, 64}, 0.25f, false,
    vertices, indices, &vertexCount, &indexCount,
    ARRAY_COUNT(vertices), ARRAY_COUNT(indices));

    
  auto BeginMeshDrawInfo = [](MeshDrawInfo& info, MaterialID id, U32 currentIndexCount) {
    info.materialID = id;
    info.indexOffset = currentIndexCount;
  };

  auto EndMeshDrawInfo = [](MeshDrawInfo& info, U32 currentIndexCount) {
    info.indexCount = currentIndexCount - info.indexOffset;
  };

  const F32 wallSpacing = 0.2f;

  {
    BeginMeshDrawInfo(data->meshDrawInfos[1], MaterialID_StoneTile00, indexCount); 
    V3 boundsMin = structureOrigin;
    V3 boundsMax = structureOrigin + structureSize;
    GenerateQuadFromPointsCW(
      V3{boundsMin.x, boundsMin.y, boundsMin.z},
      V3{boundsMax.x, boundsMin.y, boundsMin.z},
      V3{boundsMax.x, boundsMin.y, boundsMax.z},
      V3{boundsMin.x, boundsMin.y, boundsMax.z}, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));

    EndMeshDrawInfo(data->meshDrawInfos[1], indexCount);
  }

  BeginMeshDrawInfo(data->meshDrawInfos[2], MaterialID_WoodFloor00, indexCount); 
  { //Exterior Surfaces
    V3 boundsMin = structureOrigin;
    V3 boundsMax = structureOrigin + structureSize;
    GenerateQuadFromPointsCW(
      V3{boundsMin.x, boundsMin.y, boundsMin.z},
      V3{boundsMax.x, boundsMin.y, boundsMin.z},
      V3{boundsMax.x, boundsMax.y, boundsMin.z},
      V3{boundsMin.x, boundsMax.y, boundsMin.z}, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    GenerateQuadFromPointsCW(
      V3{boundsMax.x, boundsMin.y, boundsMin.z},
      V3{boundsMax.x, boundsMin.y, boundsMax.z},
      V3{boundsMax.x, boundsMax.y, boundsMax.z},
      V3{boundsMax.x, boundsMax.y, boundsMin.z}, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    GenerateQuadFromPointsCW(
      V3{boundsMax.x, boundsMin.y, boundsMax.z},
      V3{boundsMin.x, boundsMin.y, boundsMax.z},
      V3{boundsMin.x, boundsMax.y, boundsMax.z},
      V3{boundsMax.x, boundsMax.y, boundsMax.z}, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    GenerateQuadFromPointsCW(
      V3{boundsMin.x, boundsMin.y, boundsMax.z},
      V3{boundsMin.x, boundsMin.y, boundsMin.z},
      V3{boundsMin.x, boundsMax.y, boundsMin.z},
      V3{boundsMin.x, boundsMax.y, boundsMax.z}, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
#if 0
    GenerateQuadFromPointsCW(
      V3{boundsMin.x, boundsMax.y, boundsMin.z}, 
      V3{boundsMax.x, boundsMax.y, boundsMin.z}, 
      V3{boundsMax.x, boundsMax.y, boundsMax.z}, 
      V3{boundsMin.x, boundsMax.y, boundsMax.z}, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
#endif
  }
  
  { //Interior Surfaces 
    V3 boundsMin = structureOrigin + 
      V3{wallSpacing, 0, wallSpacing};
    V3 boundsMax = structureOrigin - 
      V3{wallSpacing, wallSpacing, wallSpacing} + structureSize;
    GenerateQuadFromPointsCCW(
      V3{boundsMin.x, boundsMin.y, boundsMin.z},
      V3{boundsMax.x, boundsMin.y, boundsMin.z},
      V3{boundsMax.x, boundsMax.y, boundsMin.z},
      V3{boundsMin.x, boundsMax.y, boundsMin.z}, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    GenerateQuadFromPointsCCW(
      V3{boundsMax.x, boundsMin.y, boundsMin.z},
      V3{boundsMax.x, boundsMin.y, boundsMax.z},
      V3{boundsMax.x, boundsMax.y, boundsMax.z},
      V3{boundsMax.x, boundsMax.y, boundsMin.z}, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    GenerateQuadFromPointsCCW(
      V3{boundsMax.x, boundsMin.y, boundsMax.z},
      V3{boundsMin.x, boundsMin.y, boundsMax.z},
      V3{boundsMin.x, boundsMax.y, boundsMax.z},
      V3{boundsMax.x, boundsMax.y, boundsMax.z}, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    GenerateQuadFromPointsCCW(
      V3{boundsMin.x, boundsMin.y, boundsMax.z},
      V3{boundsMin.x, boundsMin.y, boundsMin.z},
      V3{boundsMin.x, boundsMax.y, boundsMin.z},
      V3{boundsMin.x, boundsMax.y, boundsMax.z}, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    GenerateQuadFromPointsCCW(
      V3{boundsMin.x, boundsMax.y, boundsMin.z}, 
      V3{boundsMax.x, boundsMax.y, boundsMin.z}, 
      V3{boundsMax.x, boundsMax.y, boundsMax.z}, 
      V3{boundsMin.x, boundsMax.y, boundsMax.z}, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
  }
  EndMeshDrawInfo(data->meshDrawInfos[2], indexCount);
  
  CalculateSurfaceNormals(vertices, vertexCount, indices, indexCount);
  CalculateVertexTangents(vertices, indices, vertexCount, indexCount);
  CreateIndexedVertexArray3D(&data->proceduralWorld, 
    vertices, indices, vertexCount, indexCount, GL_STATIC_DRAW);

  DirectionalLight light;
  light.color = V3 { 1.0f, 1.0f, 1.0f };
  light.direction = V3 { 0.7f, 0.7f, 0.7f };

  for(size_t i = 0; i < 4; i++) {
    data->meshDrawInfos[i].vertexArrayID = data->proceduralWorld.vertexArrayID;
  }
}

void VenomModuleUpdate(GameMemory* memory) {
  ShowAssets(&memory->assets);
  GameData* data = (GameData*)memory->userdata;
  EntityArray* entityArray = &data->entityArray;

  InputState* input = &memory->inputState;

  Player& player = data->player;
  const float acceleration = 15.0f;
  const float dragCoefficent = 0.9f;
  float deltaTime = memory->gameState.deltaTime;
  if(input->isKeyDown[KEYCODE_W]) 
    player.velocity -= V3(0, 0, acceleration) * deltaTime;
  if(input->isKeyDown[KEYCODE_S]) 
    player.velocity += V3{0, 0, acceleration} * deltaTime;
  if(input->isKeyDown[KEYCODE_A]) 
    player.velocity -= V3{acceleration, 0, 0} * deltaTime;
  if(input->isKeyDown[KEYCODE_D]) 
    player.velocity += V3{acceleration, 0, 0} * deltaTime;

  Camera& camera = data->camera;
  player.position += player.velocity * deltaTime;
  player.velocity *= dragCoefficent;
  camera.position.x = Lerp(camera.position.x, player.position.x, 0.2f);
  camera.position.z = Lerp(camera.position.z, player.position.z, 0.2f);


  ImGui::Begin("Entities");
  ImGui::Columns(2);
  ImGui::Separator();
  static U64 selectedIndex = (1L << 32);
  ImGui::BeginChild("EntityList");
  for (size_t i = 0; i < ENTITY_CAPACITY; i++) {
    if (entityArray->flags[i] & EntityFlag_ACTIVE) {
      const Entity& entity = entityArray->entities[i];
      ImGui::PushID(i);
      if (ImGui::Selectable(ENTITY_TYPE_STRINGS[entity.type], selectedIndex == i)) {
        selectedIndex = i;
      }
      ImGui::PopID();
    }
  }
  
  ImGui::EndChild();
  ImGui::NextColumn();
  ImGui::BeginChild("Info");
  if (selectedIndex != (1L << 32)) {
    Entity& entity = entityArray->entities[selectedIndex];
    ImGui::DragFloat3("Position", &entity.position.x, 0.1f);
  }
  ImGui::EndChild();
  ImGui::NextColumn();
  ImGui::End();
}
void VenomModuleLoad(GameMemory* memory) {
  GameData* data = (GameData*)memory->userdata;
  V3 structureOrigin = { 0, 0.01F, 0 };
  V3 structureSize = { 8, 2.6F, 5 };


  if(!data->initalized) {

    U64 id = CreateEntity(EntityType_PointLight, &data->entityArray);
    Entity& entity = data->entityArray.entities[id];
    entity.position = V3 {4, 3.0, 3 };
    entity.pointLight.color = V3 { 1.0, 1.0, 1.0 };
    entity.pointLight.radius = 20.0f;
    entity.modelID = DEBUGModelID_Lamp;

#if 1
    {
      U64 id = CreateEntity(EntityType_PointLight, &data->entityArray);
      Entity& entity = data->entityArray.entities[id];
      entity.position = V3 {1.5, 6, 2.0 };
      entity.pointLight.color = V3 { 0.9, 0.9, 1.0 };
      entity.pointLight.radius = 20.0f;
      entity.modelID = DEBUGModelID_Lamp;
    }
#endif
  
#if 0 
    { 
      U64 id = CreateEntity(EntityType_Container, &data->entityArray);
      Entity& entity = data->entityArray.entities[id];
      entity.position = V3 { 1, 0, 1.8 };
      entity.modelID = DEBUGModelID_Stairs;
    }
#endif


    auto model = GetModelDrawable(DEBUGModelID_SmallBarrel, &memory->assets);
    auto barrelAsset = memory->assets.loadedModels[DEBUGModelID_SmallBarrel];
    V3 min = structureOrigin + V3 {0, -barrelAsset.aabb.min.y, 0};
    V3 max = structureOrigin + 
      V3 {structureSize.x, -barrelAsset.aabb.min.y, structureSize.z};
    RNGSeed seed(8);

    for (size_t i = 0; i < 4; i++) {
      U64 id = CreateEntity(EntityType_Container, &data->entityArray);
      Entity& entity = data->entityArray.entities[id];
      entity.position = RandomInRange(min, max, seed);
      entity.modelID = DEBUGModelID_SmallBarrel;
    }

    data->initalized = true;
  }
}


void VenomModuleRender(GameMemory* memory) {
  RenderState* rs = &memory->renderState;
  GameData* data = (GameData*)memory->userdata;

  //PushDrawCommand(&data->proceduralWorld, &rs->drawList);A

  //AddDirectionalLight(V3{0.7, 0.7, 0.7}, V3{1.0, 1.0, 1.0}, &rs->drawList);

  for (size_t i = 0; i < 4; i++)
    PushMeshDrawCommand(data->meshDrawInfos[i].vertexArrayID, 
      data->meshDrawInfos[i].materialID, data->meshDrawInfos[i].indexCount,
      data->meshDrawInfos[i].indexOffset, &rs->drawList);
#if 0
  for (size_t i = 0; i < 100; i ++)
    PushModelDrawCommand(DEBUGModelID_player, data->testPositions[i], &rs->drawList);
#endif

#if 0
  for (size_t i = 0; i < rs->lightingState.pointLightCount; i++) {
    PushModelDrawCommand(DEBUGModelID_Lamp, 
      rs->lightingState.pointLights[i].position,
      &rs->drawList);
  }
#endif


  EntityArray* entityArray = &data->entityArray;
  for (U64 i = 0; i < ENTITY_CAPACITY; i++) {
    if (entityArray->flags[i] & EntityFlag_ACTIVE) {
      Entity* entity = &entityArray->entities[i];
      if (entity->type == EntityType_PointLight) {
        AddShadowCastingPointLight(entity->position, entity->pointLight.color, 
          entity->pointLight.radius, &rs->drawList);
      }

      PushModelDrawCommand((DEBUGModelID)entity->modelID, 
        entity->position, &rs->drawList); 
    }
  }

  const Player& player = data->player;
  PushModelDrawCommand(DEBUGModelID_player,
    player.position, &rs->drawList);

#if 0
  for (size_t i = 0; i < 4; i++) {
   PushModelDrawCommand(DEBUGModelID_SmallBarrel, 
     data->containerPositions[i], &rs->drawList); 
  }
#endif

#if 0
  for (size_t z = 0; z < 8; z++) {
    for (size_t x = 0; x < 8; x++) {
      const V3 base = { 10, 0, 10 };
      const V3 position = base + V3{ x*2.0f, 0, z*2.0f};
      PushModelDrawCommand(DEBUGModelID_white_flower, 
       position, &rs->drawList); 
    }
  }
#endif


  //VenomRenderScene(memory, &rs->debugCamera);
  VenomRenderScene(memory, &data->camera);
}
