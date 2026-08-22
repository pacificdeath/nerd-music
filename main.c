#include "main.h"

// hooray! global variables
static State *state;

static uint64_t NextRandom() {
    uint64_t x = state->randomState;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state->randomState = x;
    return x;
}

static float NoteToFrequency(int note, int octave) {
    int semitoneIndex = note + (octave - 4) * 12;
    return 440.0f * powf(2.0f, semitoneIndex / 12.0f);
}

static uint64_t MusicalEventDurationToSampleDuration(int duration) {
    return duration * (SAMPLE_RATE * 0.3f); // TODO: should be based on BPM
}

static MusicalEvent GenerateToneEvent() {
    MusicalEvent event = {0};
    event.toneCount = 1;

    Tone tone;
    tone.octave = 4; // TODO
    tone.note = NextRandom() % NOTE_COUNT; // TODO

    switch (NextRandom() % 3) {
        case 0: event.duration = DURATION_16TH; break;
        case 1: event.duration = DURATION_8TH; break;
        case 2: event.duration = DURATION_4TH; break;
        default: PANIC(); break;
    }

    event.tones[0] = tone;

    return event;
}

static Measure GenerateMeasureWithRepeatingRhythms(const Measure *previousMeasure, int size, int times) {
    Measure measure = {0};

    int duration = 0;
    int eventsPerSize;
    for (eventsPerSize = 0; eventsPerSize < size; eventsPerSize++) {
        measure.events[eventsPerSize] = GenerateToneEvent();
        duration += measure.events[eventsPerSize].duration;
        if (duration >= size) {
            int overflow = (duration - size);
            measure.events[eventsPerSize].duration -= overflow;
            duration -= overflow;
            eventsPerSize++;
            break;
        }
    }
    ASSERT_EQ(duration * times, DURATION_WHOLE);

    for (int eventIndex = 0; eventIndex < eventsPerSize; eventIndex++) {
        // this is a duration in the first "size" that will dictate the rhythm of
        // durations with the same index relative to their "size"
        int masterDuration = measure.events[eventIndex].duration;
        for (int timeIndex = 1; timeIndex < times; timeIndex++) {
            int offsetEventIndex = (timeIndex * eventsPerSize) + eventIndex;
            // TODO: this sets duration twice
            measure.events[offsetEventIndex] = GenerateToneEvent();
            measure.events[offsetEventIndex].duration = masterDuration;
        }
    }

    measure.eventCount = eventsPerSize * times;
    if (previousMeasure != NULL) {
        measure.startSample = previousMeasure->startSample + MusicalEventDurationToSampleDuration(DURATION_WHOLE);
    } else {
        // initialization by passing null to this function
        measure.startSample = 0;
    }
    measure.position.event = measure.events[0];
    measure.position.eventPosition = 0.0f;

    return measure;
}

// TODO: there should be several of these that can generate melodies or chord progressions in different fun ways
static Measure GenerateMelodyMeasure(const Measure *previousMeasure) {
    switch (NextRandom() % 3) {
        case 0: // full random
            return GenerateMeasureWithRepeatingRhythms(previousMeasure, DURATION_WHOLE, 1);
        case 1: // rythmic motif x 2
            return GenerateMeasureWithRepeatingRhythms(previousMeasure, DURATION_HALF, 2);
        case 2: // rythmic motif x 4
            return GenerateMeasureWithRepeatingRhythms(previousMeasure, DURATION_4TH, 4);
        default:
            PANIC(); return (Measure){0};
    }
}

static Measure GenerateHarmonyMeasure(const Measure *previousMeasure) {
    // TODO:
    return (Measure){0};
}

static Track CreateTrack(float initialFrequency, int trackType) {
    Track track = {0};
    track.volume = 1.0f;
    track.frequency = initialFrequency;
    track.sineIndex = 0.0f;
    switch (trackType) {
        case TRACK_MELODY:
            track.measure = GenerateMelodyMeasure(NULL);
            break;
        // case TRACK_HARMONY:
        //     track.measure = GenerateHarmonyMeasure(NULL);
        //     break;
    }
    return track;
}

static bool FindPositionInMeasure(Measure *measure, uint64_t currentSample, MeasurePosition *result) {
    uint64_t noteStartPosition = measure->startSample;
    for (int eventIndex = 0; eventIndex < MEASURE_EVENT_CAPACITY; eventIndex++) {
        uint64_t noteDuration = MusicalEventDurationToSampleDuration(measure->events[eventIndex].duration);
        uint64_t noteEndPosition = noteStartPosition + noteDuration;
        if (noteEndPosition < currentSample) {
            // skip until we find the earliest event where the end of the note is later than current sample
            noteStartPosition += noteDuration;
            continue;
        }
        result->event = measure->events[eventIndex];
        result->eventIndex = eventIndex;
        result->eventPosition = (float)(currentSample - noteStartPosition) / (float)(noteEndPosition - noteStartPosition);
        return true;
    }

    // current sample has passed the duration of the entire measure
    return false;
}

static float MoveTowards(float current, float target) {
    return target + (current - target) * 0.9995; // TODO 0.9995?
}

static void AudioInputCallback(void *buffer, unsigned int frames) {
    uint64_t currentSample = state->currentSample;

    for (unsigned int i = 0; i < frames; i++) {
        state->bigBuffer[i] = 0;
    }

    for (int trackIndex = 0; trackIndex < TRACK_TOTAL; trackIndex++) {
        Track *track = &state->tracks[trackIndex];
        float frequency = track->frequency;
        float sineIndex = track->sineIndex;
        float volume = track->volume;
        MeasurePosition measurePosition;

        while (!FindPositionInMeasure(&track->measure, currentSample, &measurePosition)) {
            switch (trackIndex) {
                case TRACK_MELODY:
                    track->measure = GenerateMelodyMeasure(&track->measure);
                    break;
                // case TRACK_HARMONY:
                //     track->measure = GenerateHarmonyMeasure(&track->measure);
                //     break;
            }
        }

        for (unsigned int i = 0; i < frames; i++) {
            // TODO: hardcoded [0] index here, this is only the case for things like melody
            float targetFrequency = NoteToFrequency(measurePosition.event.tones[0].note, measurePosition.event.tones[0].octave);
            frequency = MoveTowards(frequency, targetFrequency);

            float targetVolume = 0.0f;
            float lowFadeThreshold = 0.2f;
            float highFadeThreshold = 1.0f - lowFadeThreshold;
            if (measurePosition.eventPosition < lowFadeThreshold) {
                targetVolume = measurePosition.eventPosition / lowFadeThreshold;
            } else if (measurePosition.eventPosition > highFadeThreshold) {
                float normalizedTimePosition = (measurePosition.eventPosition - highFadeThreshold);
                float maxTimePosition = (1.0f - highFadeThreshold);
                targetVolume = 1.0f - (normalizedTimePosition / maxTimePosition);
            } else {
                targetVolume = 1.0f;
            }

            volume = MoveTowards(volume, targetVolume);

            float incr = frequency / (float)SAMPLE_RATE;

            float x = (sineIndex < 0.5f) ? (4.0f * sineIndex - 1.0f) : (3.0f - 4.0f * sineIndex);
            state->bigBuffer[i] += (int32_t)(32000.0f * x * volume);
            sineIndex += incr;

            if (sineIndex > 1.0f) {
                sineIndex -= 1.0f;
            }
        }

        track->frequency = frequency;
        track->sineIndex = sineIndex;
        track->volume = volume;
        track->measure.position = measurePosition;
    }

    int16_t *d = (int16_t *)buffer;
    unsigned int i;
    for (i = 0; i < frames; i++) {
        d[i] = state->bigBuffer[i] / TRACK_TOTAL;
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
    //state->tracks[TRACK_HARMONY] = CreateTrack(NoteToFrequency(NOTE_A, 4), TRACK_HARMONY);
    state->tracks[TRACK_MELODY] = CreateTrack(NoteToFrequency(NOTE_A, 4), TRACK_MELODY);

    state->randomState = 90; // TODO

    PlayAudioStream(stream);

    while (!WindowShouldClose()) {
        Measure measure = state->tracks[TRACK_MELODY].measure;

        BeginDrawing();

        ClearBackground((Color){0,0,0,255});

        Rectangle measureBackground;
        measureBackground.x = 0;
        measureBackground.y = GetScreenHeight() * 0.25f;
        measureBackground.width = GetScreenWidth() - (measureBackground.x * 2);
        measureBackground.height = GetScreenHeight() - (measureBackground.y * 2);

        DrawRectangleRec(measureBackground, GRAY);

        float noteHeight = measureBackground.height / NOTE_COUNT;
        float noteOffset = 0.0f;
        for (int x = 0; x < measure.eventCount; x++) {
            MusicalEvent *currentEvent = &measure.events[x];

            // TODO: hardcoded [0] index only relevant for melody
            Tone tone = measure.events[x].tones[0];

            float noteWidth = ((float)currentEvent->duration / (float)DURATION_WHOLE) * measureBackground.width;
            Color color = measure.position.eventIndex == x ? GREEN : RED; // TODO
            Rectangle rectangle;
            rectangle.x = measureBackground.x + noteOffset;
            rectangle.y = measureBackground.y + measureBackground.height - (noteHeight * tone.note) - noteHeight;
            rectangle.width = noteWidth;
            rectangle.height = noteHeight;

            DrawRectangleRec(rectangle, color);
            if (measure.position.eventIndex == x) {
                Vector2 start = {
                    noteOffset + (measure.position.eventPosition * noteWidth),
                    measureBackground.y,
                };
                Vector2 end = {
                    start.x,
                    measureBackground.y + measureBackground.height,
                };
                DrawLineEx(start, end, 2, YELLOW); // TODO
                const char *text = TextFormat("it is %i", x, measure.position.eventIndex);
            } else {
                const char *text = TextFormat("(%i) = %i", x, measure.position.eventIndex);
            }

            noteOffset += noteWidth;
        }
        const char *t = TextFormat("%i : %.2f", measure.position.eventIndex, measure.position.eventPosition);
        dbgtext(1, t, GREEN);

        EndDrawing();
    }
}

