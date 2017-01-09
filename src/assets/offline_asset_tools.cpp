
#include "mesh_utilites.cpp"
#undef malloc
#undef free
#undef realloc
#define malloc(size) MemoryAllocate(size)
#define free(ptr) MemoryFree(ptr)
#define realloc(ptr, size) MemoryReAllocate(ptr, size)
#include "thirdparty/stb_vorbis.c"
#undef malloc
#undef free
#undef realloc
#define malloc(size) static_assert(false, "USE CUSTOM ALLOCATORS");
#define free(ptr) static_assert(false, "USE CUSTOM ALLOCATORS");
#define realloc(ptr, size) static_assert(false, "USE CUSTOM ALLOCATORS");

#include "assimp_importer.cpp"

SoundData LoadOGG(const char *filename) {
	short *output = 0;
	int channels, sample_rate;
	stb_vorbis_decode_filename(filename, &channels, &sample_rate, &output);

	int error;
	stb_vorbis *oggfile = stb_vorbis_open_filename(filename, &error, NULL);
	if (!oggfile) LogError("FAILED to load ogg file");
	U32 sampleCount = stb_vorbis_stream_length_in_samples(oggfile);
	stb_vorbis_close(oggfile);

	assert(output != nullptr);
	assert(channels == 2);
	assert(sample_rate == 48000);

	SoundData result = {};
	result.sampleCount = sampleCount;
	result.samples = output;
	return result;
}