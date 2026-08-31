// TODO: only compile on debug, measure should always be length DURATION_WHOLE
static float GetMeasureDuration(const Measure *measure) {
    ASSERT((measure->eventCount) < MEASURE_EVENT_CAPACITY);
    float duration = 0;
    for (int i = 0; i < measure->eventCount; i++) {
        duration += measure->events[i].duration;
    }
    return duration;
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

static void GenerateMeasureWithRepeatingRhythms(Measure *measure, int size, int times) {
    *measure = (Measure){0};

    int duration = 0;
    int eventsPerSize;
    for (eventsPerSize = 0; eventsPerSize < size; eventsPerSize++) {
        measure->events[eventsPerSize] = GenerateToneEvent();
        duration += measure->events[eventsPerSize].duration;
        if (duration >= size) {
            int overflow = (duration - size);
            measure->events[eventsPerSize].duration -= overflow;
            duration -= overflow;
            eventsPerSize++;
            break;
        }
    }

    ASSERT((duration * times) == DURATION_WHOLE);

    for (int eventIndex = 0; eventIndex < eventsPerSize; eventIndex++) {
        // this is a duration in the first "size" that will dictate the rhythm of
        // durations with the same index relative to their "size"
        int masterDuration = measure->events[eventIndex].duration;
        for (int timeIndex = 1; timeIndex < times; timeIndex++) {
            int offsetEventIndex = (timeIndex * eventsPerSize) + eventIndex;
            // TODO: this sets duration twice
            measure->events[offsetEventIndex] = GenerateToneEvent();
            measure->events[offsetEventIndex].duration = masterDuration;
        }
    }

    measure->eventCount = eventsPerSize * times;
}

static void GenerateMelodyMeasure(Measure *measure) {
    switch (NextRandom() % 3) {
        case 0: // full random
            GenerateMeasureWithRepeatingRhythms(measure, DURATION_WHOLE, 1);
            return;
        case 1: // rythmic motif x 2
            GenerateMeasureWithRepeatingRhythms(measure, DURATION_HALF, 2);
            return;
        case 2: // rythmic motif x 4
            GenerateMeasureWithRepeatingRhythms(measure, DURATION_4TH, 4);
            return;
        default:
            ASSERT(false);
            return;
    }
}

static void GenerateHarmonyMeasure(Measure *measure) {
    measure->eventCount = 12;
    const int octave = 4;
    for (int i = 0; i < 2; i++) {
        int offset = i * (measure->eventCount / 2);
        InitMusicalEvent(&measure->events[0 + offset], DURATION_8TH, NOTE_C, octave);
        InitMusicalEvent(&measure->events[1 + offset], DURATION_16TH, NOTE_E, octave);
        AppendMusicalEvent(&measure->events[1 + offset], NOTE_G, octave);
        InitMusicalEvent(&measure->events[2 + offset], DURATION_16TH, SILENCE, octave);

        InitMusicalEvent(&measure->events[3 + offset], DURATION_8TH, NOTE_G, octave - 1);
        InitMusicalEvent(&measure->events[4 + offset], DURATION_16TH, NOTE_E, octave);
        AppendMusicalEvent(&measure->events[4 + offset], NOTE_G, octave);
        InitMusicalEvent(&measure->events[5 + offset], DURATION_16TH, SILENCE, octave);
    }

    ASSERT(GetMeasureDuration(measure) == DURATION_WHOLE);
}

