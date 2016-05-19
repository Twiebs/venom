#ifdef _MSC_VER
#include <intrin.h>
#define rdstc() __rdstc()
#else
#include <x86intrin.h>
#endif

#include "imgui.cpp"
#include "imgui_draw.cpp"


void PushLogEntry(DebugLog *log, LogLevel level)
{
	size_t temp_buffer_length = strlen(log->temp_buffer);
	printf(log->temp_buffer);
	if (log->current_entry_count > DebugLog::ENTRY_COUNT_MAX)
	{
		assert(false);
	}

	if (log->log_buffer_used + temp_buffer_length > DebugLog::ENTRY_BUFFER_SIZE)
	{
		assert(false);
	}

	memcpy(&log->log_buffer[log->log_buffer_used], log->temp_buffer, temp_buffer_length + 1);
	log->entries[log->current_entry_count] = { level, &log->log_buffer[log->log_buffer_used] };
	log->log_buffer_used += temp_buffer_length + 1;
	log->current_entry_count += 1;
}

void BeginPeristantProfileEntry(ProfileData *profileData, const char *name)
{
	for (U32 i = 0; i < profileData->persistantEntryCount; i++)
	{
		PersistantProfilerEntry *entry = &profileData->persistantEntries[i];
		if (!strcmp(name, entry->name))
		{
			entry->elapsedCycles = __rdtsc();
			entry->elapsedTime = GetPerformanceCounterTime();
			return;
		}
	}

	assert(profileData->persistantEntryCount < PROFILE_PERSISTANT_ENTRY_COUNT_MAX);
	PersistantProfilerEntry *entry = &profileData->persistantEntries[profileData->persistantEntryCount++];
	//NOTE(Torin) This memory is never acounted for or cleaned up
	//because it doesn't matter
	size_t name_length = strlen(name);
	entry->name = (char *)malloc(name_length + 1);
	entry->name[name_length] = 0;
	memcpy(entry->name, name, name_length);
	entry->elapsedTime = 0;
	entry->elapsedCycles = __rdtsc();
}

void EndPersistantProfileEntry(ProfileData *profileData, const char *name)
{
	U64 elapsedCycles = __rdtsc();
	U64 elapsedTime = GetPerformanceCounterTime();

	for (U32 i = 0; i < profileData->persistantEntryCount; i++)
	{
		PersistantProfilerEntry *entry = &profileData->persistantEntries[i];
		if (strcmp(name, entry->name) == 0)
		{
			entry->elapsedCycles = elapsedCycles - entry->elapsedCycles;
			entry->elapsedTime = elapsedTime - entry->elapsedTime;
			float elapsedTimeInMS = ((float)entry->elapsedTime / GetPerformanceCounterFrequency()) * 1000.0f;
			entry->elapsedTimeHistory[profileData->persistantWriteIndex] = elapsedTimeInMS;
			return;
		}
	}

	assert(false);
}
