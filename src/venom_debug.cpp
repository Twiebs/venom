#ifdef _MSC_VER
#include <intrin.h>
#define rdstc() __rdstc()
#else
#include <x86intrin.h>
#endif

VenomDebugRenderSettings* GetDebugRenderSettings() {
  return &GetVenomEngineData()->renderState.debugRenderSettings;
}

VenomDebugRenderFrameInfo* GetDebugRenderFrameInfo() {
  return &GetVenomEngineData()->renderState.debugRenderFrameInfo;
}

void CreateLogEntry(LogLevel level, const char *fmt, ...) {
  SystemTime time = GetSystemTime();
  va_list args;
  va_start(args, fmt);
  auto engine = GetEngine();
  auto threadID = GetThreadID();
  auto worker = &engine->workers[threadID];
  
  U8 *writePtr = worker->stackMemory.memory + worker->stackMemory.used;
  size_t bufferSize = worker->stackMemory.size - worker->stackMemory.used;
  int bytesWritten = vsnprintf((char *)writePtr, bufferSize, fmt, args);
  assert(bytesWritten > 0 && bytesWritten < bufferSize);
  bytesWritten += 1; //Include null terminator
  DebugLog *log = &engine->debugLog;

  log->mutex.lock();
  if (level == LogLevel_ERROR) engine->unseenErrorCount++;
  else if (level == LogLevel_WARNING) engine->unseenWarningCount++;
  else if (level == LogLevel_INFO) engine->unseenInfoCount++;
  if (log->current_entry_count > DebugLog::ENTRY_COUNT_MAX) {
    assert(false);
  }

  //TODO(Torin) Serialize buffer to disk properly!
  if (log->log_buffer_used + bytesWritten > DebugLog::ENTRY_BUFFER_SIZE) {
    assert(false);
  }

  printf((const char *)writePtr);
  memcpy(&log->log_buffer[log->log_buffer_used], writePtr, bytesWritten);

  LogEntry *entry = &log->entries[log->current_entry_count];
  entry->level = level;
  entry->time = time;
  entry->text = &log->log_buffer[log->log_buffer_used];
  log->log_buffer_used += bytesWritten;
  log->current_entry_count += 1;
  log->mutex.unlock();
  va_end(args);
}