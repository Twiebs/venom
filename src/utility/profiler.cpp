
void BeginTimedBlock(const char *name) {
  auto engine = GetEngine();
  auto profileData = &engine->profileData;

  AquireLock(&profileData->lock);

  for (size_t i = 0; i < profileData->persistantEntryCount; i++) {
    PersistantProfilerEntry *entry = &profileData->persistantEntries[i];
    if (strcmp(name, entry->name) == 0) {
      entry->startTime = GetPerformanceCounterTime();
      ReleaseLock(&profileData->lock);
      return;
    }
  }

  assert(profileData->persistantEntryCount < PROFILE_PERSISTANT_ENTRY_COUNT_MAX);
  PersistantProfilerEntry *entry = &profileData->persistantEntries[profileData->persistantEntryCount++];

  //NOTE(Torin) This memory is never acounted for or cleaned up
  //This is nessecary because the provided char* will point to garbage 
  //if the game module is hotloaded 
  size_t name_length = strlen(name);
  entry->name = (char *)MemoryAllocate(name_length + 1);
  entry->name[name_length] = 0;
  memcpy(entry->name, name, name_length);
  entry->startTime = GetPerformanceCounterTime();
  ReleaseLock(&profileData->lock);
}

void EndTimedBlock(const char *name) {
  U64 currentTime = GetPerformanceCounterTime();
  auto engine = GetEngine();
  auto profileData = &engine->profileData;

  AquireLock(&profileData->lock);
  for (size_t i = 0; i < profileData->persistantEntryCount; i++) {
    PersistantProfilerEntry *entry = &profileData->persistantEntries[i];
    if (strcmp(name, entry->name) == 0) {
      U64 elapsedTime = currentTime - entry->startTime;
      F64 frequency = (F64)GetPerformanceCounterFrequency() / 1000.0;
      float elapsedTimeInMilliSeconds = elapsedTime / frequency;

      entry->elapsedTimes[entry->historyWriteIndex++] = elapsedTimeInMilliSeconds;
      if (entry->historyWriteIndex > ARRAY_COUNT(entry->elapsedTimes))
        entry->historyWriteIndex = 0;
      ReleaseLock(&profileData->lock);
      return;
    }
  }

  ReleaseLock(&profileData->lock);
  assert(false && "No matching label for profile block");
}

void BeginProfileEntry(const char *name) {
  auto engine = GetEngine();
  auto profileData = &engine->profileData;
  AquireLock(&profileData->lock);
  if (profileData->explicitEntryCount > ARRAY_COUNT(profileData->explictEntries)) {
    //TODO(Torin) Serialize!
    assert(false);
  }

  ExplicitProfilerEntry *entry = &profileData->explictEntries[profileData->explicitEntryCount++];
  ReleaseLock(&profileData->lock);
  entry->name = strdup(name);
  entry->elapsedTimeTicks = GetPerformanceCounterTime();
}

void EndProfileEntry() {
  U64 currentTime = GetPerformanceCounterTime();
  auto profileData = &GetEngine()->profileData;
  ExplicitProfilerEntry *entry = &profileData->explictEntries[profileData->explicitEntryCount - 1];
  U64 elapsedTime = currentTime - entry->elapsedTimeTicks;
  F64 frequency = (F64)GetPerformanceCounterFrequency() / 1000.0;
  entry->elapsedTimeMilliseconds = (F64)elapsedTime / frequency;
}