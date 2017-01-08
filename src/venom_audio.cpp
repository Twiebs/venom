
void PlaySound(AudioState *audioState, U32 count, U32 totalSamples, S16 *soundData) {
	assert(count > 0);
	assert(totalSamples > 0);
	assert(soundData != nullptr);

	U32 playingSoundIndex = 0;
	for (; playingSoundIndex < AUDIO_MAX_PLAYING_SOUNDS; playingSoundIndex++)
		if (!audioState->isPlaying[playingSoundIndex]) break;
	assert(audioState->isPlaying[playingSoundIndex] == false); //If all the sound slots are filled
	
	PlayingSound *sound = &audioState->playingSounds[playingSoundIndex];
	sound->totalSampleCount = totalSamples;
	sound->countToPlay = count;
	sound->soundData = soundData;
	sound->currentSamplesPlayed = 0;
	audioState->isPlaying[playingSoundIndex] = true;
}

void MixAudioOutputBuffer(AudioState *state, size_t requestedByteCount) {
	assert(requestedByteCount % 2 == 0);
	//U32 samplesToMix = requestedByteCount / AUDIO_BYTES_PER_SAMPLE;
	memset(state->outputBuffer, 0, requestedByteCount);
#if 0
	for (int i = 0; i < AUDIO_MAX_PLAYING_SOUNDS; i++)
	{
		if (state->isPlaying[i])
		{
			PlayingSound &sound = state->playingSounds[i];
			if (sound.currentSamplesPlayed >= sound.totalSampleCount) {
				sound.countToPlay -= 1;
				sound.currentSamplesPlayed = 0;
				if (sound.countToPlay == 0) {
					state->isPlaying[i] = false;
					continue;
				}
			}
			
			for (size_t n = 0; n < samplesToMix; n++)
			{
				state->outputBuffer[n] += sound.soundData[sound.currentSamplesPlayed];
				sound.currentSamplesPlayed++;
			}
		}
	}
#endif
}

