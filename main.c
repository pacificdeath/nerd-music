#include "main.h"

#include "musical_event.c"
#include "measure.c"

static float NoteToFrequency(int note, int octave) {
    int semitoneIndex = note + (octave - 4) * 12;
    return 440.0f * powf(2.0f, semitoneIndex / 12.0f);
}

static float MoveTowards(float current, float target) {
    return target + (current - target) * 0.9995; // TODO 0.9995?
}

static void AudioInputCallback(void *buffer, unsigned int frames) {
    uint64_t currentSample = state->currentSample;

    for (unsigned int i = 0; i < frames; i++) {
        state->bigBuffer[i] = 0;
    }

    int nonMutedMeasures = MEASURE_TOTAL;

    for (int measureIndex = 0; measureIndex < MEASURE_TOTAL; measureIndex++) {
        Measure *measure = &state->measures[measureIndex];

        if (HAS_FLAG(measure->flags, MEASURE_FLAG_MUTED)) {
            nonMutedMeasures--;
            continue;
        }

        while (!UpdateMeasurePosition(measure, currentSample)) {
            switch (measureIndex) {
                case MEASURE_MELODY:
                    *measure = GenerateMelodyMeasure(measure);
                    break;
                case MEASURE_HARMONY:
                    *measure = GenerateHarmonyMeasure(measure);
                    break;
            }
        }

        MusicalEvent *event = &measure->events[measure->position.eventIndex];

        for (uint8_t toneIndex = 0; toneIndex < event->toneCount; toneIndex++) {
            Tone *tone = &event->tones[toneIndex];

            uint8_t note = (uint8_t)tone->note;
            if (note == SILENCE) {
                continue;
            }

            uint8_t octave = tone->octave;
            float volume = tone->volume;
            float frequency = tone->frequency;
            float sineIndex = tone->sineIndex;

            for (unsigned int frameIndex = 0; frameIndex < frames; frameIndex++) {
                float targetVolume = 0.0f;
                float lowFadeThreshold = 0.2f;
                float highFadeThreshold = 1.0f - lowFadeThreshold;
                if (measure->position.eventPosition < lowFadeThreshold) {
                    targetVolume = measure->position.eventPosition / lowFadeThreshold;
                } else if (measure->position.eventPosition > highFadeThreshold) {
                    float normalizedTimePosition = (measure->position.eventPosition - highFadeThreshold);
                    float maxTimePosition = (1.0f - highFadeThreshold);
                    targetVolume = 1.0f - (normalizedTimePosition / maxTimePosition);
                } else {
                    targetVolume = 1.0f;
                }

                volume = MoveTowards(volume, targetVolume);

                float targetFrequency = NoteToFrequency(note, octave);
                //frequency = MoveTowards(frequency, targetFrequency);
                frequency = targetFrequency;

                float incr = frequency / (float)SAMPLE_RATE;

                float triangle = (sineIndex < 0.5f) ? (4.0f * sineIndex - 1.0f) : (3.0f - 4.0f * sineIndex);
                int32_t toneData = (int32_t)(32000.0f * triangle * volume);

                state->bigBuffer[frameIndex] += toneData / event->toneCount;

                sineIndex += incr;
                if (sineIndex > 1.0f) {
                    sineIndex -= 1.0f;
                }
            }

            tone->volume = volume;
            tone->frequency = frequency;
            tone->sineIndex = sineIndex;
        }
    }

    int16_t *d = (int16_t *)buffer;
    unsigned int i;
    for (i = 0; i < frames; i++) {
        d[i] = state->bigBuffer[i] / MEASURE_TOTAL;
    }
    state->currentSample += i;
}

static void dbgrec(int index, Color color) {
    int padding = 5;
    int side = 20;
    DrawRectangle(
        (side + padding) * index,
        padding,
        side,
        side,
        color
    );
}

static void dbgtext(int index, const char *text, Color color) {
    DrawText(text, 5, 5 + (index * 40), 20, color);
}

int main() {
    state = (State *)calloc(sizeof(State), 1);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(600, 600, "The music program");
    SetTargetFPS(60);

    InitAudioDevice();

    SetAudioStreamBufferSizeDefault(AUDIO_STREAM_SIZE);
    AudioStream stream = LoadAudioStream(SAMPLE_RATE, SAMPLE_SIZE, CHANNELS);
    SetAudioStreamCallback(stream, AudioInputCallback);

    uint8_t scale[SCALE_CAPACITY] = SCALE_HARMONIC_MINOR;
    // TODO: this here
    state->measures[MEASURE_HARMONY] = GenerateHarmonyMeasure(NULL);
    state->measures[MEASURE_MELODY] = GenerateMelodyMeasure(NULL);

    state->randomState = 90; // TODO

    PlayAudioStream(stream);

    // TODO: temporary solution probably
    const int lowestOctave = 2;
    const int highestOctave = 6;
    const int octaveCount = highestOctave - lowestOctave + 1;

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground((Color){0,0,0,255});

        for (int measureIndex = 0; measureIndex < MEASURE_TOTAL; measureIndex++) {
            Measure measure = state->measures[measureIndex];

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

            const float eventHeight = measureBackground.height / (octaveCount * NOTE_COUNT);

            float eventOffset = 0.0f;
            for (int eventIndex = 0; eventIndex < measure.eventCount; eventIndex++) {
                const MusicalEvent *event = &measure.events[eventIndex];
                float eventWidth = ((float)event->duration / (float)DURATION_WHOLE) * measureBackground.width;

                for (int toneIndex = 0; toneIndex < event->toneCount; toneIndex++) {
                    Tone tone = event->tones[toneIndex];

                    if (tone.note == SILENCE) {
                        continue;
                    }

                    Rectangle rectangle;
                    rectangle.x = measureBackground.x + eventOffset;
                    int pitch = ((int)tone.octave - lowestOctave) * NOTE_COUNT + (int)tone.note;
                    rectangle.y = measureBackground.y + measureBackground.height - (pitch + 1) * eventHeight;

                    rectangle.width = eventWidth;
                    rectangle.height = eventHeight;

                    Color color = measure.position.eventIndex == eventIndex ? GREEN : RED;

                    DrawRectangleRec(rectangle, color);
                    if (measure.position.eventIndex == eventIndex) {
                        Vector2 start = {
                            eventOffset + (measure.position.eventPosition * eventWidth),
                            measureBackground.y,
                        };
                        Vector2 end = {
                            start.x,
                            measureBackground.y + measureBackground.height,
                        };
                        DrawLineEx(start, end, 2, YELLOW); // TODO
                        const char *text = TextFormat("it is %i", event->tones[0].note);
                        dbgtext(0, text, RED);
                    } else {
                        const char *text = TextFormat("(%i) = %i", eventIndex, measure.position.eventIndex);
                    }
                }

                eventOffset += eventWidth;
            }
            const char *t = TextFormat("%i : %.2f", measure.position.eventIndex, measure.position.eventPosition);
            dbgtext(1, t, GREEN);
        }

        EndDrawing();
    }
}

