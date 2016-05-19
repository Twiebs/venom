#define MEMORY_BLOCK_MAX_CHILDREN 8

#define PushStruct(structType, memblock) (structType*)PushSize(sizeof(structType), memblock)
#define PushArray(ElementType, ElementCount, Elements,Arena) memcpy(PushSize(sizeof(ElementType) * ElementCount, Arena), Elements, sizeof(ElementType) * ElementCount)
#define ReserveArray(elementType, elementCount, memoryBlock) ((elementType *)PushSize(sizeof(elementType) * elementCount, memoryBlock))
#define AquireArrayMemory(elements, count, memory) elements = ((decltype(elements)) PushSize(sizeof(*elements) * count, memory))
#define InitSubBlock(name, block, size, parent) _SetBlockName(block, name); _SubBlock(block, size, parent)

struct MemoryBlock {
	U8* base;
	size_t size;
	size_t used;

#ifndef VENOM_RELEASE
	const char *name;
	MemoryBlock *children[MEMORY_BLOCK_MAX_CHILDREN];
	U32 childCount;
#endif
};

inline U8 *PushSize(size_t size, MemoryBlock *block) {
	assert(block->used + size <= block->size);
	U8 *result = block->base + block->used;
	block->used += size;
	return result;
}

inline void _SubBlock(MemoryBlock *child, size_t size, MemoryBlock *parent) {
	child->size = size;
	child->base = PushSize(size, parent);
	child->used = 0;
#ifndef VENOM_RELEASE
	assert(parent->childCount < MEMORY_BLOCK_MAX_CHILDREN);
	parent->children[parent->childCount++] = child;
#endif
}

inline void _SetBlockName(MemoryBlock *block, const char *name) {
	block->name = name;
}
