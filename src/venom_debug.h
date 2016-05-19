enum LogLevel {
	LogLevel_ERROR,
  LogLevel_WARNING,
	LogLevel_INFO,
	LogLevel_DEBUG,
};

static const char *LOGLEVEL_TAG[] = {
	"[ERROR]",
  "[WARNING]",
	"[INFO]",
	"[DEBUG]"
};

struct LogEntry {
	LogLevel level;
	char *text;
};

static const V4 LOGLEVEL_COLOR[] = {
	V4(1.0f, 0.0f, 0.0f, 1.0f),
  V4(1.0f, 1.0f, 0.0f, 1.0f),
	V4(1.0f, 1.0f, 1.0f, 1.0f),
	V4(0.6f, 0.6f, 0.6f, 1.0f),
};

struct DebugLog {
	static const U32 OPENGL_ID_COUNT_MAX = 64;
	static const U32 ENTRY_COUNT_MAX = 4096;
	static const U32 ENTRY_BUFFER_SIZE = MEGABYTES(2);
	static const U32 TEMP_BUFFER_SIZE = KILOBYTES(256);

	LogEntry entries[ENTRY_COUNT_MAX];
	char temp_buffer[TEMP_BUFFER_SIZE];
	char log_buffer[ENTRY_BUFFER_SIZE];
	U32 logged_opengl_ids[OPENGL_ID_COUNT_MAX];

	size_t current_entry_count;
	size_t log_buffer_used;
	U32 logged_opengl_id_count;
};


struct ExplicitProfilerEntry 
{
	const char *name;
	U64 elapsedCPUCycles;
	float elapsedFrameTime;
};

#define PROFILER_ELAPSED_TIME_HISTORY_COUNT 128
#define PROFILE_PERSISTANT_ENTRY_COUNT_MAX 16

struct PersistantProfilerEntry
{
	char *name;
	U64 elapsedCycles;
	U64 elapsedTime;
	float elapsedTimeHistory[PROFILER_ELAPSED_TIME_HISTORY_COUNT];
};

struct ProfileData
{
	PersistantProfilerEntry persistantEntries[PROFILE_PERSISTANT_ENTRY_COUNT_MAX];
	ExplicitProfilerEntry explictEntries[256];
	U32 persistantEntryCount;
	U32 explicitEntryCount;
	U32 persistantWriteIndex;
};

void PushLogEntry(DebugLog *log, LogLevel level);//, const char *message, ...);

void BeginPersistantProfileEntry(ProfileData *data, const char *name);
void EndPersistantProfileEntry(ProfileData *data, const char *name);

#if 0
void BeginSingleEntry(const char *name);
void BeginPersistantEntry(const char *name);
void EndPersistantEntry(const char *name);
#endif


