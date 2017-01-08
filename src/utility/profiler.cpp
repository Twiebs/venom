
void BeginTimedBlock(const char *name) {
  auto engine = GetEngine();
  auto profileData = &engine->profileData;

  /* LOCK */ profileData->mutex.lock();

  for (size_t i = 0; i < profileData->persistantEntryCount; i++) {
    PersistantProfilerEntry *entry = &profileData->persistantEntries[i];
    if (strcmp(name, entry->name) == 0) {
      entry->startTime = GetPerformanceCounterTime();
      /* UNLOCK */profileData->mutex.unlock();
      return;
    }
  }

  assert(profileData->persistantEntryCount < PROFILE_PERSISTANT_ENTRY_COUNT_MAX);
  PersistantProfilerEntry *entry = &profileData->persistantEntries[profileData->persistantEntryCount++];

  //NOTE(Torin) This memory is never acounted for or cleaned up
  //This is nessecary because the provided char* will point to garbage 
  //if the game module is hotloaded 
  size_t name_length = strlen(name);
  entry->name = (char *)malloc(name_length + 1);
  entry->name[name_length] = 0;
  memcpy(entry->name, name, name_length);
  entry->startTime = GetPerformanceCounterTime();

  profileData->mutex.unlock();
}

void EndTimedBlock(const char *name) {
  U64 currentTime = GetPerformanceCounterTime();
  auto engine = GetEngine();
  auto profileData = &engine->profileData;

  profileData->mutex.lock();
  for (size_t i = 0; i < profileData->persistantEntryCount; i++) {
    PersistantProfilerEntry *entry = &profileData->persistantEntries[i];
    if (strcmp(name, entry->name) == 0) {
      U64 elapsedTime = currentTime - entry->startTime;
      float elapsedTimeInNanoseconds = (elapsedTime / (GetPerformanceCounterFrequency()));
      float elapsedTimeInMilliseconds = elapsedTimeInNanoseconds / 1000000.0f;
      entry->elapsedTimes[entry->historyWriteIndex++] = elapsedTimeInMilliseconds;
      if (entry->historyWriteIndex > ARRAY_COUNT(entry->elapsedTimes))
        entry->historyWriteIndex = 0;
      profileData->mutex.unlock();
      return;
    }
  }

  profileData->mutex.unlock();
  assert(false && "No matching label for profile block");
}