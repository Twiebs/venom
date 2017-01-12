
#ifndef VENOM_TRACK_MEMORY
inline U8 *MemoryAllocateDefault(size_t size) {
  auto engine = GetEngine();
  engine->currentMemoryAllocated += size;
#undef malloc
  return (U8 *)malloc(size);
#define malloc(size) static_assert("DONT USE MALLOC");
}

inline U8 *MemoryReAllocateDefault(void * ptr, size_t size) {
#undef realloc
  return (U8 *)realloc(ptr, size);
#define realloc(ptr, size) static_assert(false, "DONT USE REALLOC");
}

inline void MemoryFreeDefault(void *ptr) {
#undef free
  free(ptr);
#define free(ptr) static_assert("DONT USE FREE");
}

#else //VENOM_TRACK_MEMORY

inline U8 *MemoryAllocateTracked(size_t size, size_t lineNumber, const char *file, const char *function) {
#undef malloc
  U8 *result = (U8 *)malloc(size);
#define malloc(size) static_assert("DONT USE MALLOC");
  auto engine = GetEngine();
  AquireLock(&engine->memoryAllocationLock);
  MemoryAllocationEntry *entry = &engine->memoryAllocations[engine->memoryAllocationCount++];
  ReleaseLock(&engine->memoryAllocationLock);
  entry->lineNumber = lineNumber;
  entry->filename = file;
  entry->procedureName = function;
  entry->allocationSize = size;
  entry->memoryAddress = (uintptr_t)result;
  return result;
}

inline U8 *MemoryReAllocateTracked(void *ptr, size_t size, size_t lineNumber, const char *file, const char *function) {
  if (ptr == nullptr) {
    return MemoryAllocateTracked(size, lineNumber, file, function);
  }

#undef realloc
  U8 *result = (U8 *)realloc(ptr, size);
#define realloc(ptr, size) static_assert(false, "DONT USE REALLOC");
  auto engine = GetEngine();
  AquireLock(&engine->memoryAllocationLock);
  for (size_t i = 0; i < engine->memoryAllocationCount; i++) {
    MemoryAllocationEntry *entry = &engine->memoryAllocations[i];
    if (entry->memoryAddress == (uintptr_t)ptr) {
      ReleaseLock(&engine->memoryAllocationLock);
      entry->lineNumber = lineNumber;
      entry->filename = file;
      entry->procedureName = function;
      entry->allocationSize = size;
      entry->memoryAddress = (uintptr_t)result;
      return result;
    }
  }

  ReleaseLock(&engine->memoryAllocationLock);
  assert(false);
  return nullptr;
}

inline void MemoryFreeTracked(void *ptr, size_t lineNumber, const char *file, const char *function) {
#undef free
  if (ptr == nullptr) return;
  auto engine = GetEngine();
  AquireLock(&engine->memoryAllocationLock);
  for (size_t i = 0; i < engine->memoryAllocationCount; i++) {
    MemoryAllocationEntry *entry = &engine->memoryAllocations[i];
    if (entry->memoryAddress == (uintptr_t)ptr) {
      engine->memoryAllocations[i] = engine->memoryAllocations[engine->memoryAllocationCount - 1];
      engine->memoryAllocationCount -= 1;
      ReleaseLock(&engine->memoryAllocationLock);
      free(ptr);
      return;
    }
  }
  ReleaseLock(&engine->memoryAllocationLock);
  assert(false);

#define free(ptr) static_assert("DONT USE FREE");
}

#endif