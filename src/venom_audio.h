static const int AUDIO_SAMPLES_PER_SECOND = 48000;
static const int AUDIO_BYTES_PER_SAMPLE = sizeof(S16);
static const int AUDIO_OUTPUT_BUFFER_SIZE = AUDIO_SAMPLES_PER_SECOND * AUDIO_BYTES_PER_SAMPLE;

#define AUDIO_MAX_PLAYING_SOUNDS 16

struct SoundData
{
	U32 sampleCount;
	S16 *samples;
};

struct PlayingSound
{
	U32 totalSampleCount;
	U32 currentSamplesPlayed;
	U32 countToPlay;
	S16 *soundData;
};

struct AudioState
{
	bool isPlaying[AUDIO_MAX_PLAYING_SOUNDS];
	PlayingSound playingSounds[AUDIO_MAX_PLAYING_SOUNDS];
	S16 outputBuffer[AUDIO_OUTPUT_BUFFER_SIZE];
};

void PlaySound(AudioState *audioState, U32 count, U32 totalSamples, S16 *soundData);
void MixAudioOutputBuffer(AudioState *state, size_t requestedByteCount);