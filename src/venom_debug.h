#include "thirdparty/imgui.h"

VenomDebugRenderSettings* GetDebugRenderSettings(); 
VenomDebugRenderFrameInfo* GetDebugRenderFrameInfo(); 

enum LogLevel {
	LogLevel_ERROR,
  LogLevel_WARNING,
	LogLevel_INFO,
	LogLevel_DEBUG,
};

static const char *LogLevelNames[] = {
	"ERROR",
  "WARNING",
	"INFO",
	"DEBUG"
};

static const V4 LOGLEVEL_COLOR[] = {
  V4(1.0f, 0.0f, 0.0f, 1.0f),
  V4(1.0f, 1.0f, 0.0f, 1.0f),
  V4(1.0f, 1.0f, 1.0f, 1.0f),
  V4(0.6f, 0.6f, 0.6f, 1.0f),
};

struct LogEntry {
	LogLevel level;
  SystemTime time;
	char *text;
};

struct DebugLog {
	static const U32 OPENGL_ID_COUNT_MAX = 64;
	static const U32 ENTRY_COUNT_MAX = 4096;
	static const U32 ENTRY_BUFFER_SIZE = MEGABYTES(2);
	static const U32 TEMP_BUFFER_SIZE = KILOBYTES(256);

  std::mutex mutex;
	LogEntry entries[ENTRY_COUNT_MAX];
	char log_buffer[ENTRY_BUFFER_SIZE];
	U32 logged_opengl_ids[OPENGL_ID_COUNT_MAX];

	size_t current_entry_count;
	size_t log_buffer_used;
	U32 logged_opengl_id_count;
};