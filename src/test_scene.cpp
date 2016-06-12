#define VENOM_ENTITY_METALIST_MACRO EntityTypeList
#define VENOM_ENTITY_STRUCT Entity
#define VENOM_MATERIAL_LIST_FILE "asset_list.h"

#include "venom_module.cpp"

struct Player {
  V3 position;
  V3 velocity;
  F32 cooldownTime;
};

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

struct GameData {
  Camera camera;
  Structure structures[2];
  //EntityArray entityArray;
  EntityContainer entityContainer;
  EditorData editorData;
  bool initalized;
  Player player;

  IndexedVertexArray proceduralMesh;
};

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

void GenerateRandomBSPTree(const Rectangle& root, U32 divisionCount, 
  RNGSeed& seed, Rectangle* results, U32* resultsWritten, U32 currentDepth = 0) 
{
  static const F32 MINIMUM_SPLIT_PERCENTAGE = 0.4F;
  static const F32 MINIMUM_SPLIT_SIZE = 2.5F;

  S32 splitDirection = Random01U64(seed); 
  F32 splitPercentange = RandomInRange(MINIMUM_SPLIT_PERCENTAGE, 
    1.0F - MINIMUM_SPLIT_PERCENTAGE, seed);

  Rectangle childA, childB;
  if(splitDirection == 0) { //Vertical Split
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

  if((currentDepth + 1) == divisionCount) {
    results[*resultsWritten+0] = childA;
    results[*resultsWritten+1] = childB;
    *resultsWritten += 2;
    return;
  }

  GenerateRandomBSPTree(childA, divisionCount, seed, 
    results, resultsWritten, currentDepth + 1);
  GenerateRandomBSPTree(childB, divisionCount, seed, 
    results, resultsWritten, currentDepth + 1);
}

void GenerateStructure(const V3 structureBoundMin, const V3 structureBoundMax, 
  IndexedVertexArray* vertexArray, MeshDrawInfo* drawInfo) 
{
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
      V3{boundsMin.x, boundsMin.y, boundsMin.z},
      V3{boundsMax.x, boundsMin.y, boundsMin.z},
      V3{boundsMax.x, boundsMin.y, boundsMax.z},
      V3{boundsMin.x, boundsMin.y, boundsMax.z}, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    EndMeshDrawInfo(drawInfo[0], indexCount);
  }

  BeginMeshDrawInfo(drawInfo[1], MaterialID_WoodFloor00, indexCount); 
  { //Exterior Surfaces
    V3 boundsMin = structureBoundMin;
    V3 boundsMax = structureBoundMax;
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
  }
  
  { //Interior Surfaces 
    V3 boundsMin = structureBoundMin + 
      V3{wallSpacing, 0, wallSpacing};
    V3 boundsMax = structureBoundMax - 
      V3{wallSpacing, wallSpacing, wallSpacing}; 
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

  EndMeshDrawInfo(drawInfo[1], indexCount);
  CalculateSurfaceNormals(vertices, vertexCount, indices, indexCount);
  CalculateVertexTangents(vertices, indices, vertexCount, indexCount);
  CreateIndexedVertexArray3D(vertexArray, vertices, indices, 
    vertexCount, indexCount, GL_STATIC_DRAW);
}

void VenomModuleStart(GameMemory* memory) {
  SystemInfo* sys = &memory->systemInfo;
  GameData* data = PushStruct(GameData, &memory->mainBlock);
  memory->userdata = data;
  RenderState* rs = &memory->renderState;

#if 1
  {
    ModelData data = {};
    data = ImportExternalModelData(VENOM_ASSET_FILE("axis.obj"), 0);
    FILE* file = fopen("test.txt", "wb");
    assert(file != 0);
    for(size_t i = 0; i < data.meshData.vertexCount; i++) {
      fprintf(file, "V3{%ff, %ff, %ff},\n", data.meshData.vertices[i].position.x,
        data.meshData.vertices[i].position.y, data.meshData.vertices[i].position.z);
    }
    fprintf(file, "\n");
    for(size_t i = 0; i < data.meshData.indexCount; i++) {
      fprintf(file, "%u,", data.meshData.indices[i]);
      if(i % 3 == 0) fprintf(file, "\n");
    }
  }
#endif


  InitializeCamera(&data->camera, 45*DEG2RAD, 0.1f, 100.0f,
    sys->screen_width, sys->screen_height);
  data->camera.position = {4, 10, 2};
  data->camera.pitch = -89.99f*DEG2RAD;
  data->camera.yaw = -90.0f*DEG2RAD;

  EntityContainerInit(&data->entityContainer, 1024, 8);
  
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
    Rectangle bounds = bspTree[(i*2) + selector];
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

    V3 boundMin = {bounds.minX, 0, bounds.minY};
    V3 boundMax = {bounds.minX + width, 3, bounds.minY + height};

    GenerateStructure(boundMin, boundMax,
      &data->structures[i].vertexArray, &data->structures[i].meshDrawInfos[0]);
    for(size_t n = 0; n < 4; n++) {
      data->structures[i].meshDrawInfos[n].vertexArrayID = 
        data->structures[i].vertexArray.vertexArrayID;
    }
  }

  {
#if 0
    EntityArray* entityArray = &data->entityArray;
    auto CreateEntityGrid = [entityArray](V3 origin, DEBUGModelID id, float spacing, size_t count) {
      for (size_t y = 0; y < count; y++) {
        for (size_t x = 0; x < count; x++) {
          U32 entityID = CreateEntity(EntityType_StaticObject, entityArray);
          Entity& entity = entityArray->entities[entityID];
          entity.position = origin + V3 { x * spacing, 0, y * spacing }; 
          entity.modelID = id;
        }
      }
    };

    CreateEntityGrid(V3{0,0,0}, DEBUGModelID_UtahTeapot, 2, 4);

#endif
  }
  { 
    V3 boundsMin = { -2, 0, -2 };
    V3 boundsMax = { 16, 0, 16 }; 
    Vertex3D vertices[4];
    U32 indices[6];
    U32 vertexCount = 0, indexCount = 0;
    GenerateQuadFromPointsCW(
      V3{boundsMin.x, boundsMin.y, boundsMin.z},
      V3{boundsMax.x, boundsMin.y, boundsMin.z},
      V3{boundsMax.x, boundsMin.y, boundsMax.z},
      V3{boundsMin.x, boundsMin.y, boundsMax.z}, 1,
      vertices, indices, &vertexCount, &indexCount,
      ARRAY_COUNT(vertices), ARRAY_COUNT(indices));
    CalculateSurfaceNormals(vertices, vertexCount, indices, indexCount);
    CalculateVertexTangents(vertices, indices, vertexCount, indexCount);
    CreateIndexedVertexArray3D(&data->proceduralMesh, vertices, indices, 
      vertexCount, indexCount, GL_STATIC_DRAW);
  }
}

struct VenomWorldFileHeader {
  U64 verifier;
  U64 entityCount;
} __attribute((packed));

struct EntityInfo {
  U32 type;
  V3 position;
  V3 rotation;
  U32 modelID;
} __attribute((packed));

struct NewEntityInfo {
  U32 type;
  Entity entity;
} __attribute((packed));

inline int
FindMatchingString(const char *source, const char **list, size_t listLength) {
  for(size_t i = 0; i < listLength; i++)
    if(strcmp(source, list[i]) == 0) return (int)i;
  return -1;
}

static inline
void WriteWorldFile(const char *filename, EntityContainer *container, 
    AssetManifest *manifest) {
  vs::BeginFileWrite(filename);
  EntityBlock* block = container->firstAvaibleBlock;
  for(size_t i = 0; i < container->capacityPerBlock; i++){
    if(block->flags[i] & EntityFlag_PRESENT) {
      Entity* e = &block->entities[i];
      vs::BeginGroupWrite("entity");
      vs::WriteString("type", EntityTypeStrings[block->types[i]]);
      vs::WriteFloat32("position", 3, &e->position);
      vs::WriteFloat32("rotation", 3, &e->rotation);
      vs::WriteString("modelName", GetModelAsset(e->modelID, manifest)->name);
      switch(block->types[i]) {
        case EntityType_PointLight: {
          vs::WriteFloat32("color", 3, &e->pointLight.color);
        } break;
      }
      vs::EndGroupWrite(); 
    }
  }
  vs::EndFileWrite();
}

static inline
void ReadWorldFile(const char *filename, EntityContainer *container, AssetManifest *manifest)
{
  vs::BeginFileRead(filename);
  while(vs::BeginGroupRead()){
    char temp[128];
    vs::ReadString("type", temp, sizeof(temp));
    int type = FindMatchingString(temp, EntityTypeStrings, EntityType_Count);
    if(type == -1) { //TODO(Torin) Should probably do somthing smarter if this fails!
      LogError("Invalid EntityType when reading world file");
      vs::EndGroupRead();
      continue;
    }

#if 1
    Entity *e = CreateEntity((EntityType)type, container);
    vs::ReadFloat32("position", 3, &e->position);
    vs::ReadFloat32("rotation", 3, &e->rotation);
    vs::ReadString("modelName", temp, sizeof(temp));
    S64 modelID = GetModelID(temp, manifest);
    if(modelID == -1) {
      LOG_ERROR("Entity created with unkown model %s", temp);
    } else {
      e->modelID = (U32)modelID;
    }

    switch(type) {
      case EntityType_PointLight: {
        vs::ReadFloat32("color", 3, &e->pointLight.color);
      };
    }
#endif



    //LogDebug(temp);

    vs::EndGroupRead();
  }
  vs::EndFileRead();
}

static void
FuzzyFind(const char *input, const char** searchList, size_t searchListCount, bool *output){
  if (input[0] == 0) { 
    memset(output, 1, searchListCount);
    return;
  }

  size_t inputLength = strlen(input);

  for (size_t i = 0; i < searchListCount; i++) {
    int finalScore = -1;
    const char* rootSearchPos = searchList[i];

    while(*rootSearchPos != 0){
      int score = -1;
      int lastMatchIndex = -1;
      int currentInputIndex = 0;
      const char* searchChar = rootSearchPos;
      while(*searchChar != 0){
        if(input[currentInputIndex] == 0) break;
        if(*searchChar == input[currentInputIndex] || 
            ((input[currentInputIndex] >= 'a' && input[currentInputIndex] <= 'z') &&
              (input[currentInputIndex] - ('a' - 'A')) == (*searchChar))) 
        {
          score += (currentInputIndex - lastMatchIndex);
          lastMatchIndex = currentInputIndex;
          currentInputIndex++;
        }

        searchChar++;
      }

      if(currentInputIndex != (int)inputLength) score = -1;
      if(score != -1 && (score < finalScore || finalScore == -1))
        finalScore = score;
      rootSearchPos++;
    }

    if(finalScore != -1){
      output[i] = 1;
    } else {
      output[i] = 0;
    }
  }
}


void VenomModuleUpdate(GameMemory* memory) {
  ShowAssetManifest(&memory->assetManifest);
  ShowEventOverlay(memory->debugData);
  GameData* data = (GameData*)memory->userdata;
  EntityContainer* entityContainer = &data->entityContainer;
  EditorData* editorData = &data->editorData;
  RenderState* rs = &memory->renderState;
  AssetManifest *manifest = &memory->assetManifest;

  const SystemInfo* sys = &memory->systemInfo;
  const InputState* input = &memory->inputState;

  Player& player = data->player;
#if 0
  const float acceleration = 15.0f;
  const float dragCoefficent = 0.9f;
  const F32 COOLDOWN_TIME = 0.2f;
  float deltaTime = memory->gameState.deltaTime;
  if(input->isKeyDown[KEYCODE_W]) 
    player.velocity -= V3(0, 0, acceleration) * deltaTime;
  if(input->isKeyDown[KEYCODE_S]) 
    player.velocity += V3{0, 0, acceleration} * deltaTime;
  if(input->isKeyDown[KEYCODE_A]) 
    player.velocity -= V3{acceleration, 0, 0} * deltaTime;
  if(input->isKeyDown[KEYCODE_D]) 
    player.velocity += V3{acceleration, 0, 0} * deltaTime;

  F32 cursorDisplacementX = input->cursorPosX - (sys->screen_width * 0.5f);
  F32 cursorDisplacementY = input->cursorPosY - (sys->screen_height * 0.5f);
  V2 displacementVector = { cursorDisplacementX, cursorDisplacementY };
  displacementVector = Normalize(displacementVector);

  const F32 BULLET_SPEED = 5000.0f;
  if(input->isButtonDown[MOUSE_LEFT] && player.cooldownTime <= 0.0f) {
    U64 id = CreateEntity(EntityType_Bullet, entityArray);
    Entity* entity = &entityArray->entities[id];
    entity->position = player.position + V3(0.0f, 0.4f, 0.0f);
    entity->velocity = { displacementVector.x * BULLET_SPEED, 
      0, displacementVector.y * BULLET_SPEED };
    entity->bullet.decayTime = 2.0f;
    entity->bullet.lightColor = V3(0.9, 0.0, 0.2);
    entity->bullet.bulletRadius = 0.05f;
    entity->bullet.lightRadius= 1.0;
    player.cooldownTime += COOLDOWN_TIME; 
  }

  player.cooldownTime -= deltaTime;
  if (player.cooldownTime < 0.0f)
    player.cooldownTime = 0.0f;

  Camera& camera = data->camera;
  player.position += player.velocity * deltaTime;
  player.velocity *= dragCoefficent;
  camera.position.x = Lerp(camera.position.x, player.position.x, 0.2f);
  camera.position.z = Lerp(camera.position.z, player.position.z, 0.2f);
#endif

  EditorData* editor = &data->editorData;

  if (input->isButtonDown[MOUSE_RIGHT]) {
    MoveCameraWithFPSControls(&memory->renderState.debugCamera,
      &memory->inputState, memory->deltaTime);
  } else {

#if 0
    if(input->isKeyDown[KEYCODE_X]) {
      if(editor->selectedEntities.count > 0) {
        for(size_t i = 0; i < editor->selectedEntities.count; i++){
          EntityIndex index = {};
          index.blockIndex = 0;
          index.slotIndex = editor->selectedEntities[i]; 
          DestroyEntity(index, entityContainer);
        }
        editor->selectedEntities.count = 0;
      }
    }
#endif
  }

  if(input->isKeyDown[KEYCODE_X]){
    editor->transformConstraints = EditorTransformConstraint_XAxis;
  } else if(input->isKeyDown[KEYCODE_Y]){
    editor->transformConstraints = EditorTransformConstraint_YAxis;
  } else if(input->isKeyDown[KEYCODE_Z]){
    editor->transformConstraints = EditorTransformConstraint_ZAxis;
  }



  Camera* camera = &rs->debugCamera;
  ShowCameraInfo(camera);

  if(input->isKeyDown[KEYCODE_SHIFT])
    editor->selectMode = EditorSelectMode_Append;
  else if (input->isKeyDown[KEYCODE_LCONTROL])
    editor->selectMode = EditorSelectMode_Remove;
  else editor->selectMode = EditorSelectMode_Default;


  if(input->isButtonDown[MOUSE_LEFT] && !ImGui::IsMouseHoveringAnyWindow()){
    editor->activeCommand = EditorCommand_Select; 
  }

  if(input->isKeyDown[KEYCODE_E]){
    editor->activeCommand = EditorCommand_MeshEdit;
  }

  if(input->isKeyDown[KEYCODE_G]) {
    if(input->isKeyDown[KEYCODE_SHIFT])
      editor->activeCommand = EditorCommand_Grab;
    else editor->activeCommand = EditorCommand_Project;
  }
  
  if(input->isKeyDown[KEYCODE_CAPSLOCK]) editor->activeCommand = EditorCommand_None;
  ProcessEditorCommand(editor, camera, memory, entityContainer);

#if 0
  static char textBuffer[1024];
  static bool stringMatches[DEBUGModelID_COUNT];
  ImGui::Begin("FuzzyFinder");
  ImGui::InputText("Input", textBuffer, ARRAY_COUNT(textBuffer));
  FuzzyFind(textBuffer, MODEL_ASSET_NAMES, ARRAY_COUNT(MODEL_ASSET_NAMES),
    stringMatches);

  for(size_t i = 0; i < ARRAY_COUNT(stringMatches); i++){
    if(stringMatches[i]) {
      ImGui::Text(MODEL_ASSET_NAMES[i]);
    }
  }
  
  ImGui::End();
#endif


  ImGui::Begin("Entities");
  ImGui::Text("Active Editor Command: %s", EditorCommandNames[editor->activeCommand]);
  ImGui::Text("SelectedEntityCount: %d", (int)editor->selectedEntities.count);
  
  if(ImGui::Button("Save World")) {
    char temp[512];
    sprintf(temp, "silly%d.world", (int)time(0));
    WriteWorldFile(temp, entityContainer, manifest);
    WriteWorldFile("silly.world", entityContainer, manifest);
  }

  ImGui::SameLine();
  if(ImGui::Button("Load World")) {
    ReadWorldFile("silly.world", entityContainer, &memory->assetManifest);
  }

  ImGui::SameLine();
  if(ImGui::Button("CreateEntity")){
    CreateEntity(EntityType_StaticObject, entityContainer);
  }

  if(ImGui::Button("DestroyEntity")) {
    for(size_t i = 0; i < editor->selectedEntities.count; i++){
      EntityIndex index = {};
      index.blockIndex = 0;//TODO(Torin)
      index.slotIndex = editor->selectedEntities[i];
      DestroyEntity(index, entityContainer);
    }
  }

  ImGui::Columns(2);
  ImGui::Separator();
  ImGui::BeginChild("EntityList");

  //TODO(Torin)
  EntityBlock* block = entityContainer->firstAvaibleBlock;
  for(size_t i = 0; i < entityContainer->capacityPerBlock; i++){
    if(block->flags[i] & EntityFlag_PRESENT){
      const Entity& entity = block->entities[i];
      ImGui::PushID(i);

      size_t listIndex = 0;
      bool containsIndex = editor->selectedEntities.ContainsValue(i, &listIndex);
      if(ImGui::Selectable(EntityTypeStrings[block->types[i]], containsIndex)) {
        UpdateEditorSelection(i, editor, entityContainer, &memory->assetManifest);
      }
      ImGui::PopID();
    }
  }

  
  ImGui::EndChild();
  ImGui::NextColumn();
  ImGui::BeginChild("Info");
  if(editor->selectedEntities.count > 0){
    //Entity& entity = block->entities[editor->selectedEntityIndex];
    ImGui::DragFloat3("Position", &editor->currentGroupPosition.x, 0.1f);
    ImGui::DragFloat3("Rotation", &editor->currentGroupRotation.x, 0.1f);
    
   if(editor->selectedEntities.count == 1) {
    Entity *entity = GetEntity(editor->selectedEntities[0], entityContainer);
    ModelAsset *modelAsset = GetModelAsset(entity->modelID, manifest);
    if(ImGui::Button(modelAsset->name)) {
      ImGui::OpenPopup("ModelSelect");
    }
    if (ImGui::BeginPopup("ModelSelect")) {
      for (size_t i = 0; i < manifest->modelAssets.count; i++) {
        if (ImGui::Selectable(manifest->modelAssets[i].name)) {
          entity->modelID = i;
        }
      }
      ImGui::EndPopup();
     }
   } 

  }
  ImGui::EndChild();
  ImGui::NextColumn();
  ImGui::End();


  ImGui::ShowTestWindow();
}

void VenomModuleLoad(GameMemory* memory) {
  GameData* data = (GameData*)memory->userdata;
  V3 structureOrigin = { 0, 0.01F, 0 };
  V3 structureSize = { 8, 2.6F, 5 };
}


void VenomModuleRender(GameMemory* memory) {
  RenderState* rs = &memory->renderState;
  GameData* data = (GameData*)memory->userdata;
  EditorData* editorData = &data->editorData;
  EditorData* editor = &data->editorData;
  
  const F32 deltaTime = memory->deltaTime;
  EntityContainer* entityContainer = &data->entityContainer;
  EntityBlock* block = entityContainer->firstAvaibleBlock;
  for(U64 i = 0; i < entityContainer->capacityPerBlock; i++) {
    if(block->flags[i] & EntityFlag_PRESENT) {
      Entity* entity = &block->entities[i];
      if(block->types[i] == EntityType_PointLight) {
        AddShadowCastingPointLight(entity->position, entity->pointLight.color, 
          entity->pointLight.radius, &rs->drawList);
      } 


      if(entity->modelID != 0 && (block->flags[i] & EntityFlag_Visible)){
        if(editor->selectedEntities.ContainsValue(i)){
          PushOutlinedModelDrawCommand(entity->modelID, 
            &rs->drawList, entity->position, entity->rotation); 
        } else {
          PushModelDrawCommand(entity->modelID, 
            &rs->drawList, entity->position, entity->rotation); 
        }
        auto modelAsset = GetModelAsset(entity->modelID, &memory->assetManifest);
        V3 boundsSize = Abs(modelAsset->aabb.max - modelAsset->aabb.min);

        AddWireframeBox(&rs->drawList,
          entity->position - (boundsSize * 0.5f),
          entity->position + (boundsSize * 0.5f));
      }
    }
  }

#if 0
  for (size_t i = 0; i < ARRAY_COUNT(data->structures); i++) {
    for (size_t n = 0; n < 2; n++) {
      PushMeshDrawCommand(data->structures[i].meshDrawInfos[n].vertexArrayID, 
        data->structures[i].meshDrawInfos[n].materialID, 
        data->structures[i].meshDrawInfos[n].indexCount,
        data->structures[i].meshDrawInfos[n].indexOffset, &rs->drawList);
    }
  }
  const Player& player = data->player;
  PushModelDrawCommand(DEBUGModelID_player,
    player.position, &rs->drawList);
#endif 

  VenomRenderScene(memory, &rs->debugCamera);
  //VenomRenderScene(memory, &data->camera);
}
