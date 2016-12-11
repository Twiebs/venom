
static inline
EntityBlock* AllocateEntityBlock(const size_t entityCount){
  size_t requiredStructMemory = Align8(sizeof(EntityBlock)); 
  size_t requiredFlagMemory = Align8(sizeof(*EntityBlock::flags) * entityCount);
  size_t requiredTypeMemory = Align8(sizeof(*EntityBlock::types) * entityCount);
  size_t requiedEntityMemory = Align8(sizeof(*EntityBlock::entities) * entityCount);

  uint8_t* memory = (uint8_t *)calloc(requiredStructMemory + requiredFlagMemory + 
    requiredTypeMemory + requiedEntityMemory, 1);

  EntityBlock* result = (EntityBlock*)memory;
  memory += requiredStructMemory;
  result->flags = (U32*)memory; 
  memory += requiredFlagMemory;
  result->types = (EntityType*)memory;
  memory += requiredTypeMemory;
  result->entities = (VENOM_ENTITY_STRUCT*)memory;
  result->currentAliveEntityCount = 0;

  return result;
}

Entity* CreateEntity(EntityType type,
EntityIndex* outIndex, EntityContainer* container) 
{
  EntityBlock* block = container->firstAvaibleBlock;
  if(block->currentAliveEntityCount + 1 > container->capacityPerBlock){
    assert(false); //TODO(Torin)
  }

  //TODO(Torin)Use a freelist instead of searching
  VENOM_ENTITY_STRUCT* entity = 0;
  fori(container->capacityPerBlock){
    if((block->flags[i] & EntityFlag_PRESENT) == 0){
      entity = &block->entities[i];
      block->types[i] = type;
      block->flags[i] = (EntityFlag)(EntityFlag_PRESENT | EntityFlag_VISIBLE);
      outIndex->blockIndex = 0;
      outIndex->slotIndex = i;
      block->currentAliveEntityCount++;
      break;
    }
  }
  return entity;
}

void assign_model_to_entity(EntityIndex index, Asset_ID id, AssetManifest *manifest, EntityContainer *container) {
  EntityBlock *block = container->blocks[index.blockIndex];
  Entity *entity = &block->entities[index.slotIndex];
  ModelAsset *asset = GetModelAsset(id, manifest);
  if (asset->data.jointCount > 0) {
    entity->animation_state.joint_count = asset->data.jointCount;
    for (size_t i = 0; i < entity->animation_state.joint_count; i++) {
      entity->animation_state.joints = asset->data.joints;
    }
  } 

  entity->modelID = id;
}

void EntityContainerInit(EntityContainer* container, 
    U32 entityCountPerBlock, U32 initalBlockCount) {
  container->capacityPerBlock = entityCountPerBlock;
  container->blocks = (EntityBlock**)malloc(initalBlockCount * sizeof(EntityBlock*));
  container->blocks[0] = AllocateEntityBlock(entityCountPerBlock);
  container->firstAvaibleBlock = container->blocks[0];
  container->currentBlockCount = 1;
  container->currentBlockCapacity = initalBlockCount;
}


Entity* GetEntity(EntityIndex index, EntityContainer* container) {
  EntityBlock *block = container->blocks[index.blockIndex];
  return &block->entities[index.slotIndex];
}

Entity *GetEntity(U32 index, EntityContainer *container) {
  EntityIndex entity_index = {};
  entity_index.slotIndex = index;
  return GetEntity(entity_index, container);
}

