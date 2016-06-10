static const U32 INVALID_ENTITY_INDEX = ((((U64)1) << 32) - 1);


#define EntityTypeList \
  _(StaticObject) \
  _(Container) \
  _(PointLight) \
  _(Bullet) \

struct StaticObject {
  AABB aabb;
};

struct PointLightEntity {
  V3 color;
  F32 radius;
};

struct Bullet {
  F32 decayTime;
  F32 bulletRadius;
  F32 lightRadius;
  F32 lightIntensity;
  V3 lightColor;
};

struct Entity {
  V3 position;
  V3 rotation;

  V3 velocity;
  U32 modelID;

  union {
    PointLightEntity pointLight;
    Bullet bullet;
  };
};

enum EntityType {
#define _(type) EntityType_##type,
EntityTypeList
#undef _
EntityType_Count
};

static const char* EntityTypeStrings[] = {
#define _(type) #type,
EntityTypeList
#undef _
};

enum EntityFlag {
  EntityFlag_PRESENT = 1 << 0,
};

struct EntityIndex {
  U32 blockIndex;
  U32 slotIndex;
};

struct EntityBlock {
  EntityFlag* flags;
  EntityType* types;
  Entity* entities;
  U32 currentAliveEntityCount;
  //EntityIndex* entityDataIndex;
};

struct EntityDataBlock {
  void* entityData;
  size_t entityDataSize;
  size_t entityCount;
};

struct EntityContainer {
  EntityBlock** blocks;
  EntityBlock* firstAvaibleBlock;
  U32 capacityPerBlock;
  U32 currentBlockCount;
  U32 currentBlockCapacity;
};

Entity* CreateEntity(EntityType type, EntityIndex* outIndex, EntityContainer* entities);
void DestroyEntity(EntityType type, EntityContainer* entities);
Entity* GetEntity(U32 entityIndex, EntityContainer* container);
