#include "main.h"

#include "music_buffer.c"
#include "musical_event.c"
#include "measure.c"

static float NoteToFrequency(int note, int octave) {
    int semitoneIndex = note + (octave - 4) * 12;
    return 440.0f * powf(2.0f, semitoneIndex / 12.0f);
}

static float MoveTowards(float current, float target, float multiplier) {
    return target + (current - target) * multiplier;
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

            if (HAS_FLAG(measure->flags, MEASURE_FLAG_MUTED)) {
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
            bool isAudioBackBufferPrepared = atomic_load(&sharedState->isAudioBackBufferPrepared);
            if (!isAudioBackBufferPrepared) {
                // there seems to be no more music in the world
                goto NoMusicLeft;
            }

            SwapBuffers(&sharedState->audioBackBufferIndex, &audioThreadState->audioFrontBufferIndex);
            musicBuffer = GetAudioFrontBuffer();
            atomic_store(&sharedState->isAudioBackBufferPrepared, false);
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

            const int eventIndex = atomic_load(&measurePlaybackState->eventIndex);
            MusicalEvent *event = &measure->events[eventIndex];

            const unsigned int eventStartSample = measurePlaybackState->eventStartSample;
            const unsigned int eventEndSample = measurePlaybackState->eventEndSample;

            for (uint8_t toneIndex = 0; toneIndex < event->toneCount; toneIndex++) {
                Tone *tone = &event->tones[toneIndex];

                float sineIndex = tone->sineIndex;
                float frequency = NoteToFrequency((uint8_t)tone->note, tone->octave);

                float incr = frequency / (float)SAMPLE_RATE;

                for (unsigned int i = 0; i < samplesToRender; i++) {
                    unsigned int sample = measureFrameIndex + i;
                    unsigned int eventAge = sample - eventStartSample;
                    unsigned int eventRemaining = eventEndSample - sample;
                    float volume = 1.0f;

                    if (eventAge < EVENT_FADE_SAMPLES) {
                        volume = (float)eventAge / EVENT_FADE_SAMPLES;
                    } else if (eventRemaining < EVENT_FADE_SAMPLES) {
                        volume = (float)eventRemaining / EVENT_FADE_SAMPLES;
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
        if (!HAS_FLAG(measure->flags, MEASURE_FLAG_MUTED)) {
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

void update() {
    bool isAudioBackBufferPrepared = atomic_load(&sharedState->isAudioBackBufferPrepared);

    if (!isAudioBackBufferPrepared) {
        // the audio thread has swapped the audio front and back buffers,
        // this means we have to swap the mirror buffers as well
        SwapBuffers(
            &state->mirrorBackBufferIndex,
            &state->mirrorFrontBufferIndex
        );

        // regenerate into audio back buffer
        MusicBuffer *audioBackBuffer = GetAudioBackBuffer();
        for (int measureIndex = 0; measureIndex < MEASURE_TOTAL; measureIndex++) {
            Measure *measure = &audioBackBuffer->measures[measureIndex];
            switch (measureIndex) {
                case MEASURE_MELODY:
                    GenerateMelodyMeasure(measure);
                    break;
                case MEASURE_HARMONY:
                    GenerateHarmonyMeasure(measure);
                    break;
            }
        }

        // copy back buffer into mirror
        MusicBuffer *mirrorBackBuffer = GetMirrorBackBuffer();
        *mirrorBackBuffer = *audioBackBuffer;

        // signal that the audio thread can start using this thing
        atomic_store(&sharedState->isAudioBackBufferPrepared, true);
    }
}

void render() {
    BeginDrawing();

    ClearBackground((Color){0,0,0,255});

    const MusicBuffer *visualBuffer = GetMirrorFrontBuffer();

    for (int measureIndex = 0; measureIndex < MEASURE_TOTAL; measureIndex++) {
        const Measure *measure = &visualBuffer->measures[measureIndex];
        const MeasurePlaybackState *measurePlaybackState = &sharedState->measurePlaybackStates[measureIndex];

        const float cursorXPosition = atomic_load(&measurePlaybackState->cursorXPosition);
        const int currentEventIndex = atomic_load(&measurePlaybackState->eventIndex);

        Rectangle measureBackground;
        measureBackground.x = 0;
        measureBackground.y = (GetScreenHeight() / MEASURE_TOTAL) * measureIndex;
        measureBackground.width = GetScreenWidth();
        measureBackground.height = (GetScreenHeight() / MEASURE_TOTAL);

        Color backgroundColor;
        if (measureIndex == 0) {
            backgroundColor = DARKGREEN;
        } else {
            backgroundColor = DARKBLUE;
        }

        DrawRectangleRec(measureBackground, backgroundColor);
        DrawRectangleLinesEx(measureBackground, 2, BLACK);

        const float eventHeight = measureBackground.height / (SEQUENCER_OCTAVE_COUNT * NOTE_COUNT);

        float eventOffset = 0.0f;
        for (int eventIndex = 0; eventIndex < measure->eventCount; eventIndex++) {
            const MusicalEvent *event = &measure->events[eventIndex];
            float eventWidth = ((float)event->duration / (float)DURATION_WHOLE) * measureBackground.width;

            bool isCursorAtEvent = eventIndex == currentEventIndex;

            for (int toneIndex = 0; toneIndex < event->toneCount; toneIndex++) {
                Tone tone = event->tones[toneIndex];

                if (tone.note != SILENCE) {
                    Rectangle rectangle;
                    rectangle.x = measureBackground.x + eventOffset;
                    int pitch = ((int)tone.octave - SEQUENCER_LOWEST_OCTAVE) * NOTE_COUNT + (int)tone.note;
                    rectangle.y = measureBackground.y + measureBackground.height - (pitch + 1) * eventHeight;

                    rectangle.width = eventWidth;
                    rectangle.height = eventHeight;

                    Color color = isCursorAtEvent ? GREEN : RED;
                    DrawRectangleRec(rectangle, color);
                }
            }

            if (isCursorAtEvent) {
                Vector2 start = {
                    eventOffset + (cursorXPosition * eventWidth),
                    measureBackground.y,
                };
                Vector2 end = {
                    start.x,
                    measureBackground.y + measureBackground.height,
                };
                DrawLineEx(start, end, 2, YELLOW); // TODO
            }

            eventOffset += eventWidth;
        }
    }

    EndDrawing();
}

int main() {
    state = (State *)calloc(sizeof(State), 1);
    sharedState = (SharedState *)calloc(sizeof(SharedState), 1);
    audioThreadState = (AudioThreadState *)calloc(sizeof(AudioThreadState), 1);

    InitMusicBuffers();

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(600, 600, "The music program");
    SetTargetFPS(60);

    InitAudioDevice();

    SetAudioStreamBufferSizeDefault(AUDIO_STREAM_SIZE);
    AudioStream stream = LoadAudioStream(SAMPLE_RATE, SAMPLE_SIZE, CHANNELS);
    SetAudioStreamCallback(stream, AudioInputCallback);

    uint8_t scale[SCALE_CAPACITY] = SCALE_HARMONIC_MINOR;

    state->randomState = 90; // TODO

    PlayAudioStream(stream);

    while (!WindowShouldClose()) {
        update();
        render();
    }
}

