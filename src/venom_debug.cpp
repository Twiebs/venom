#ifdef _MSC_VER
#include <intrin.h>
#define rdstc() __rdstc()
#else
#include <x86intrin.h>
#endif

#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_demo.cpp"





VenomDebugRenderSettings* GetDebugRenderSettings() {
  return &GetVenomEngineData()->renderState.debugRenderSettings;
}

VenomDebugRenderFrameInfo* GetDebugRenderFrameInfo() {
  return &GetVenomEngineData()->renderState.debugRenderFrameInfo;
}

void PushLogEntry(VenomDebugData *data, LogLevel level) {
  if (level == LogLevel_ERROR) data->unseenErrorCount++;
  else if (level == LogLevel_WARNING) data->unseenWarningCount++;

  DebugLog* log = &data->debugLog;
	size_t temp_buffer_length = strlen(log->temp_buffer);
	printf(log->temp_buffer);
	if (log->current_entry_count > DebugLog::ENTRY_COUNT_MAX) {
		assert(false);
	}

	if (log->log_buffer_used + temp_buffer_length > DebugLog::ENTRY_BUFFER_SIZE) {
		assert(false);
	}

	memcpy(&log->log_buffer[log->log_buffer_used], log->temp_buffer, temp_buffer_length + 1);
	log->entries[log->current_entry_count] = { level, &log->log_buffer[log->log_buffer_used] };
	log->log_buffer_used += temp_buffer_length + 1;
	log->current_entry_count += 1;
}

void __BeginProfileEntry(ProfileData *profileData, const char *name){
	for(size_t i = 0; i < profileData->persistantEntryCount; i++){
		PersistantProfilerEntry *entry = &profileData->persistantEntries[i];
		if(!strcmp(name, entry->name)){
			entry->startTime = GetPerformanceCounterTime();
			return;
		}
	}

	assert(profileData->persistantEntryCount < PROFILE_PERSISTANT_ENTRY_COUNT_MAX);
	PersistantProfilerEntry *entry = 
    &profileData->persistantEntries[profileData->persistantEntryCount++];

	//NOTE(Torin) This memory is never acounted for or cleaned up
  //This is nessecary because the provided char* will point to garbage 
  //if the game module is hotloaded 
	size_t name_length = strlen(name);
	entry->name = (char *)malloc(name_length + 1);
	entry->name[name_length] = 0;
	memcpy(entry->name, name, name_length);
	entry->startTime = GetPerformanceCounterTime();
}

void __EndProfileEntry(ProfileData *profileData, const char *name){
	U64 currentTime = GetPerformanceCounterTime();
	for(size_t i = 0; i < profileData->persistantEntryCount; i++) {
		PersistantProfilerEntry *entry = &profileData->persistantEntries[i];
		if(strcmp(name, entry->name) == 0){
			U64 elapsedTime = currentTime - entry->startTime;
			float elapsedTimeInNanoseconds = (elapsedTime / (GetPerformanceCounterFrequency()));
      float elapsedTimeInMilliseconds = elapsedTimeInNanoseconds / 1000000.0f;
			entry->elapsedTimes[entry->historyWriteIndex++] = elapsedTimeInMilliseconds;
      if(entry->historyWriteIndex > ARRAY_COUNT(entry->elapsedTimes)) 
        entry->historyWriteIndex = 0;
			return;
		}
	}

	assert(false && "No matching label for profile block");
}

static inline
void BeginProfileEntryHook(const char* name) {
  VenomDebugData* debugData = GetDebugData();
  __BeginProfileEntry(&debugData->profileData, name);
}

static inline
void EndProfileEntryHook(const char* name) {
  VenomDebugData* debugData = GetDebugData();
  __EndProfileEntry(&debugData->profileData, name);
}

#ifndef VENOM_DISABLE_PROFILER
#define DEBUG_BeginProfileEntry(name) BeginProfileEntryHook(name) 
#define DEBUG_EndProfileEntry(name) EndProfileEntryHook(name) 
#else//VENOM_DISABLE_PROFILER
#define DEBUG_BeginProfileEntry(name)
#define DEBUG_EndProfileEntry(name)
#endif//VENOM_DISABLE_PROFILER