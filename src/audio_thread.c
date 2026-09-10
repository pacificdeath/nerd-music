typedef struct AudioThreadState {
    unsigned int currentSample;
    int32_t bigBuffer[AUDIO_STREAM_SIZE];
} AudioThreadState;

static AudioThreadState *audioThreadState = NULL;

static float NoteToFrequency(int note) {
    float semitoneIndex = note - 48.0f;
    return 440.0f * powf(2.0f, semitoneIndex / 12.0f);
}

static bool UpdateMeasurePosition(Measure *measure, int measureIndex, unsigned int currentSample) {
    ASSERT(measureIndex < MEASURE_TOTAL);

    unsigned int eventStartSample = 0;
    for (int eventIndex = 0; eventIndex < measure->eventCount; eventIndex++) {
        unsigned int eventDuration = MusicalEventDurationToSampleDuration(measure->events[eventIndex].duration);
        unsigned int eventEndSample = eventStartSample + eventDuration;
        if (currentSample >= eventEndSample) {
            // skip elapsed events
            eventStartSample = eventEndSample;
            continue;
        }

        MeasurePlaybackState *measurePlaybackState = &sharedState->measurePlaybackStates[measureIndex];

        float cursorXPosition = (float)(currentSample - eventStartSample) / (float)(eventEndSample - eventStartSample);

        // For visualization on main thread
        atomic_store_explicit(&measurePlaybackState->eventIndex, eventIndex, memory_order_relaxed);
        atomic_store_explicit(&measurePlaybackState->cursorXPosition, cursorXPosition, memory_order_relaxed);
        measurePlaybackState->eventStartSample = eventStartSample;
        measurePlaybackState->eventEndSample = eventEndSample;

        return true;
    }

    // current sample has passed the duration of the entire measure
    return false;
}

static void AudioInputCallback(void *buffer, unsigned int frames) {
    for (unsigned int i = 0; i < frames; i++) {
        audioThreadState->bigBuffer[i] = 0;
    }

    MusicBuffer *musicBuffer = GetAudioFrontBuffer();

    // frame indices relative to this callback, these do NOT reset on buffer swaps
    unsigned int frameIndices[MEASURE_TOTAL] = {0};
    // frame indices relative to the current music buffer, these DO reset on buffer swaps
    unsigned int measureFrameIndices[MEASURE_TOTAL] = {0};
    // once all the samples of a measure (or plural if a buffer swap happened), they are marked "obtained"
    bool measureSamplesObtained[MEASURE_TOTAL] = {0};

    for (int measureIndex = 0; measureIndex < MEASURE_TOTAL; measureIndex++) {
        measureFrameIndices[measureIndex] = audioThreadState->currentSample;
    }

    while (true) {
        bool allMeasureSamplesObtained = true;

        // measures might have differing event-boundaries, taking different amount of loop cycles before a buffer swap
        // this is to ensure buffer swaps happens only when all measures are ready for it
        bool shouldBufferSwap = true;

        unsigned int measureSamplesToRender[MEASURE_TOTAL] = {0};
        // loop inteded to fill measureSamplesToRender[]
        for (int measureIndex = 0; measureIndex < MEASURE_TOTAL; measureIndex++) {
            if (measureSamplesObtained[measureIndex]) continue;
            allMeasureSamplesObtained = false;

            Measure *measure = &musicBuffer->measures[measureIndex];

            if (hasFlag(measure->flags, MEASURE_FLAG_MUTED)) {
                measureSamplesObtained[measureIndex] = true;
                continue;
            }

            const unsigned int measureFrameIndex = measureFrameIndices[measureIndex];

            // UpdateMeasurePosition returns wheter or not we are still inside the music buffer
            // note that a buffer swap will not happen until all measures are ready for it
            if (UpdateMeasurePosition(measure, measureIndex, measureFrameIndex)) {
                shouldBufferSwap = false;
            }

            const MeasurePlaybackState *measurePlaybackState = &sharedState->measurePlaybackStates[measureIndex];

            unsigned int eventEndSample = measurePlaybackState->eventEndSample;

            measureSamplesToRender[measureIndex] = MIN(
                frames - frameIndices[measureIndex],
                (unsigned int)(eventEndSample - measureFrameIndex)
            );
        }

        if (allMeasureSamplesObtained) {
            // all the work has been completed
            break;
        }

        if (shouldBufferSwap) {
            bool isAudioBackBufferPrepared = atomic_load_explicit(&sharedState->isAudioBackBufferPrepared, memory_order_acquire);
            if (!isAudioBackBufferPrepared) {
                // there seems to be no more music in the world
                goto NoMusicLeft;
            }

            SwapBuffers(&sharedState->audioBackBufferIndex, &sharedState->audioFrontBufferIndex);
            musicBuffer = GetAudioFrontBuffer();
            atomic_store_explicit(&sharedState->isAudioBackBufferPrepared, false, memory_order_release);
            for (int measureIndex = 0; measureIndex < MEASURE_TOTAL; measureIndex++) {
                measureFrameIndices[measureIndex] = 0;
            }

            continue;
        }

        for (int measureIndex = 0; measureIndex < MEASURE_TOTAL; measureIndex++) {
            if (measureSamplesObtained[measureIndex]) continue;

            Measure *measure = &musicBuffer->measures[measureIndex];
            unsigned int measureFrameIndex = measureFrameIndices[measureIndex];
            unsigned int frameIndex = frameIndices[measureIndex];

            const unsigned int samplesToRender = measureSamplesToRender[measureIndex];

            const MeasurePlaybackState *measurePlaybackState = &sharedState->measurePlaybackStates[measureIndex];

            const int eventIndex = atomic_load_explicit(&measurePlaybackState->eventIndex, memory_order_relaxed);
            MusicalEvent *event = &measure->events[eventIndex];

            const unsigned int eventStartSample = measurePlaybackState->eventStartSample;
            const unsigned int eventEndSample = measurePlaybackState->eventEndSample;

            const unsigned int eventDuration =  MusicalEventDurationToSampleDuration(event->duration);

            // TODO: configurable:
            const float eventFadeSamples = eventDuration * 0.3f;

            for (int toneIndex = 0; toneIndex < event->toneCount; toneIndex++) {
                Tone *tone = &event->tones[toneIndex];

                float sineIndex = tone->sineIndex;
                float frequency = NoteToFrequency(tone->note);

                float incr = frequency / (float)SAMPLE_RATE;

                for (unsigned int i = 0; i < samplesToRender; i++) {
                    unsigned int sample = measureFrameIndex + i;
                    unsigned int eventAge = sample - eventStartSample;
                    unsigned int eventRemaining = eventEndSample - sample;
                    float volume = 1.0f;

                    if (eventAge < eventFadeSamples) {
                        volume = (float)eventAge / eventFadeSamples;
                    } else if (eventRemaining < eventFadeSamples) {
                        volume = (float)eventRemaining / eventFadeSamples;
                    }

                    float triangle = (sineIndex < 0.5f) ? (4.0f * sineIndex - 1.0f) : (3.0f - 4.0f * sineIndex);

                    int32_t toneData = (int32_t)(32000.0f * triangle * volume);

                    audioThreadState->bigBuffer[frameIndex + i] += toneData / event->toneCount;

                    sineIndex += incr;

                    if (sineIndex >= 1.0f) {
                        sineIndex -= 1.0f;
                    }
                }

                tone->sineIndex = sineIndex;
                tone->frequency = frequency;
            }

            frameIndex += samplesToRender;
            ASSERT(frameIndex <= frames);
            frameIndices[measureIndex] += samplesToRender;
            measureFrameIndices[measureIndex] += samplesToRender;

            if (frameIndex == frames) {
                measureSamplesObtained[measureIndex] = true;
            }
        }
    }

NoMusicLeft:

    int nonMutedMeasures = 0;
    for (int measureIndex = 0; measureIndex < MEASURE_TOTAL; measureIndex++) {
        const Measure *measure = &musicBuffer->measures[measureIndex];
        if (!hasFlag(measure->flags, MEASURE_FLAG_MUTED)) {
            nonMutedMeasures++;
        }
    }

    if (nonMutedMeasures == 0) {
        return;
    }

    int16_t *output = (int16_t *)buffer;

    for (unsigned int i = 0; i < frames; i++) {
        output[i] = audioThreadState->bigBuffer[i] / nonMutedMeasures;
    }

    // all the measures end at the same sample so we can just use the first index
    audioThreadState->currentSample = measureFrameIndices[0];
}

