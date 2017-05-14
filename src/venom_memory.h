
#define Align4(x) (((size_t)(x) + 0x3) & (~0x3))
#define Align8(x) (((size_t)(x) + 0x7) & (~0x7))
#define Align16(x) (((size_t)(x) + 0xF) & (~0xF))


#define MEMORY_BLOCK_MAX_CHILDREN 8

#define PushStruct(structType, memblock) (structType*)PushSize(sizeof(structType), memblock)
#define PushArray(ElementType, ElementCount, Elements,Arena) memcpy(PushSize(sizeof(ElementType) * ElementCount, Arena), Elements, sizeof(ElementType) * ElementCount)
#define ReserveArray(elementType, elementCount, memoryBlock) ((elementType *)PushSize(sizeof(elementType) * elementCount, memoryBlock))
#define AquireArrayMemory(elements, count, memory) elements = ((decltype(elements)) PushSize(sizeof(*elements) * count, memory))
#define InitSubBlock(name, block, size, parent) _SetBlockName(block, name); _SubBlock(block, size, parent)

#define StaticArray(TElement, TSize) \
struct { TElement data[TSize]; size_t count; } 

struct DynamicSizedStack {
  U8 *memory;
  size_t used;
  size_t size;
};

struct StaticSizedStack {
  U8 *memory;
  size_t used;
  size_t size;
};

struct EngineGlobals {
  DynamicSizedStack frameStack; //TODO(Torin) This needs to be thread safe
                                //Or create one for each thread and the id of the frame stack needs to be stored with
                                //the pointer... There is no way it will ever use more than 4GBs so use two U32s 
                                //one the index of the frame stack and the other the offset into the memory
} static globals;


inline U8 *StackPush(StaticSizedStack *stack, size_t size);
//Returns offset from the front of the stack
inline size_t StackPush(DynamicSizedStack *stack, size_t size);

namespace Memory {




  U32 FrameStackPush(size_t size) {
    return (U32)StackPush(&globals.frameStack, size);
  }

  U8 *FrameStackPointer(U32 offset) {
    return globals.frameStack.memory + offset;
  }

  void FrameStackClear() {
    globals.frameStack.used = 0;
  }
};

U8 *StackPush(StaticSizedStack *stack, size_t size) {
  assert(stack->used + size < stack->size);
  U8 *result = stack->memory + stack->used; 
  stack->used += size;
  return result;
}

//Returns offset from the front of the stack
size_t StackPush(DynamicSizedStack *stack, size_t size) {
  if (stack->used + size > stack->size) {
    stack->memory = (U8 *)MemoryReAllocate(stack->memory, stack->size + 4096);
    stack->size = stack->size + 4096;
  }

  size_t result = stack->used;
  stack->used += size;
  return result;
}



template<typename TElement>
struct DynamicArray {
  TElement *data;
  size_t count;
  size_t capacity;

  DynamicArray() {
    data = 0;
    count = 0;
    capacity = 0;
  }

  ~DynamicArray() {
    if (data != 0) MemoryFree(data);
  }

  TElement& operator[](size_t index){
    assert(index < capacity);
    return data[index];
  }

  TElement *AddElement() {
    if (count + 1 > capacity) Resize(capacity + 10);
    TElement *element = &data[count++];
    memset(element, 0x00, sizeof(TElement));
    return element;
  }

  void PushBack(TElement element) {
    if (count + 1 > capacity) Resize(capacity + 10);
    data[count++] = element;
  }

  void RemoveOrdered(size_t index) {
    assert(index < count);
    size_t countToMove = (count - index - 1);
    memmove(data + index, data + index + 1, countToMove);
    count--;
  }

  void RemoveUnordered(size_t index) {
    assert(index < count);
    data[index] = data[count-1];
    count--;
  }

  void RemoveFirstValueUnordered(TElement e) {
    for(size_t i = 0; i < count; i++) {
      if(data[i] == e) {
        RemoveUnordered(i);
        return;
      }
    }
  }
 
  int ContainsValue(TElement value){
    for(size_t i = 0; i < count; i++)
      if(data[i] == value) return 1;
    return 0;
  }

  int ContainsValue(TElement value, size_t *outIndex) {
    for(size_t i = 0; i < count; i++)
      if(data[i] == value) { 
        *outIndex = i;
        return 1;
      }
    return 0;
  }

  void Resize(size_t newCapacity) {
    assert(newCapacity > count);
    TElement *newData = (TElement *)MemoryAllocate(newCapacity * sizeof(TElement));
    memset(newData, 0x00, sizeof(TElement) * newCapacity);
    if (data != 0) {
      memcpy(newData, data, capacity * sizeof(TElement));
      MemoryFree(data);
    }
    data = newData;
    capacity = newCapacity;
  }

  TElement& LastElement() {
    return data[count - 1];
  }

  void ClearAndDestruct() {
    for (size_t i = 0; i < count; i++) {
      TElement *element = &data[i];
      element->~TElement();
    }
  }

};


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
