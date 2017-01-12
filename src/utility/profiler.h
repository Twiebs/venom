
#define PROFILER_ELAPSED_TIME_HISTORY_COUNT 128
#define PROFILE_PERSISTANT_ENTRY_COUNT_MAX 16

struct PersistantProfilerEntry {
  char* name;
  U64 startTime;
  float elapsedTimes[PROFILER_ELAPSED_TIME_HISTORY_COUNT];
  U64 historyWriteIndex;
};

struct ExplicitProfilerEntry {
  char *name;
  U64 elapsedCPUCycles;
  U64 elapsedTimeTicks;
  float elapsedTimeMilliseconds;
};

struct ProfileData {
  SpinLock lock;
  PersistantProfilerEntry persistantEntries[PROFILE_PERSISTANT_ENTRY_COUNT_MAX];
  ExplicitProfilerEntry explictEntries[256];
  U32 persistantEntryCount;
  U32 explicitEntryCount;
};

void EndTimedBlock(const char *name);
void BeginTimedBlock(const char *name);

void BeginProfileEntry(const char *name);
void EndProfileEntry();