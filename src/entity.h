#pragma once

#define EntityTypeList \
	EntityTypeEntry(ParticleSystem) \
	EntityTypeEntry(DebugEntity) \

#define EntityTypeEntry(entityType) EntityType_##entityType,
enum EntityType { EntityTypeList EntityType_COUNT };
#undef EntityTypeEntry

#define EntityTypeEntry(entityType) #entityType,
static const char *ENTITY_TYPE_STRING[] = { EntityTypeList };
#undef EntityTypeEntry

enum EntityFlag
{
	EntityFlag_ALIVE = 1 << 0,
};

struct EntityArray
{
	void *entities;
	U32 *flags;
	U32 count;
	U32 capacity;
};

#define InitalizeEntityArray(entityType, capacity, array, arena) _InternalInitalizeEntityArray(array, capacity, sizeof(entityType), arena)
#define CreateEntity(entityType, entityArray) (entityType *)_InternalCreateEntity(sizeof(entityType), entityArray)

inline void _InternalInitalizeEntityArray(EntityArray *array, U32 capacity, size_t entity_type_size, MemoryBlock *arena)
{
	array->entities = PushSize(entity_type_size * capacity, arena);
	array->flags = (U32*)PushSize(sizeof(U32) * capacity, arena);
	memset(array->flags, 0, sizeof(U32) * capacity);
	array->capacity = capacity;
}

inline void *_InternalCreateEntity(size_t entity_type_size, EntityArray *array)
{
	assert(array->count + 1 <= array->capacity);
	void *result = (void*)((uintptr_t)array->entities + (array->count * entity_type_size));
	array->count += 1;
	return result;
}

struct DebugEntity
{
	V3 position;
	struct
	{
		V3 min;
		V3 max;
	} size;
	V4 color;
};

struct Particle
{
	V3 position;
};

struct ParticleSystem
{
	Particle *particles;
	U32 particleCount;
};

struct Player
{
	V3 pos;
	V3 vel;
	float angle;
	bool is_grounded = false;
};

#define ENTITY_COUNT 20000
struct GameEntities
{
	Player player;

	union
	{
		struct
		{
#define EntityTypeEntry(entityType) EntityArray entityType##Array;
			EntityTypeList
#undef EntityTypeEntry
		};
		EntityArray entityArrays[EntityType_COUNT];
	};
};
