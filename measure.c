MeasurePosition CreateMeasurePosition(int eventIndex, float eventPosition) {
    return (MeasurePosition) {
        .eventIndex = eventIndex,
        .eventPosition = eventPosition,
    };
}

// TODO: only compile on debug, measure should always be length DURATION_WHOLE
static float GetMeasureDuration(const Measure *measure) {
    ASSERT((measure->eventCount) < MEASURE_EVENT_CAPACITY);
    float duration = 0;
    for (int i = 0; i < measure->eventCount; i++) {
        duration += measure->events[i].duration;
    }
    return duration;
}

static bool UpdateMeasurePosition(Measure *measure, uint64_t currentSample) {
    uint64_t noteStartPosition = measure->startSample;
    for (int eventIndex = 0; eventIndex < MEASURE_EVENT_CAPACITY; eventIndex++) {
        uint64_t noteDuration = MusicalEventDurationToSampleDuration(measure->events[eventIndex].duration);
        uint64_t noteEndPosition = noteStartPosition + noteDuration;
        if (noteEndPosition < currentSample) {
            // skip until we find the earliest event where the end of the note is later than current sample
            noteStartPosition += noteDuration;
            continue;
        }
        measure->position.eventIndex = eventIndex;
        measure->position.eventPosition = (float)(currentSample - noteStartPosition) / (float)(noteEndPosition - noteStartPosition);
        return true;
    }

    // current sample has passed the duration of the entire measure
    return false;
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

    ASSERT((duration * times) == DURATION_WHOLE);

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
    measure.position.eventIndex = 0;
    measure.position.eventPosition = 0.0f;

    return measure;
}

static Measure GenerateMelodyMeasure(const Measure *previousMeasure) {
    switch (NextRandom() % 3) {
        case 0: // full random
            return GenerateMeasureWithRepeatingRhythms(previousMeasure, DURATION_WHOLE, 1);
        case 1: // rythmic motif x 2
            return GenerateMeasureWithRepeatingRhythms(previousMeasure, DURATION_HALF, 2);
        case 2: // rythmic motif x 4
            return GenerateMeasureWithRepeatingRhythms(previousMeasure, DURATION_4TH, 4);
        default:
            ASSERT(false); return (Measure){0};
    }
}

static Measure GenerateHarmonyMeasure(const Measure *previousMeasure) {
    Measure measure;
    if (previousMeasure != NULL) {
        measure.startSample = previousMeasure->startSample + MusicalEventDurationToSampleDuration(DURATION_WHOLE);
    } else {
        // initialization by passing null to this function
        measure.startSample = 0;
    }
    measure.eventCount = 12;
    const int octave = 4;
    for (int i = 0; i < 2; i++) {
        int offset = i * (measure.eventCount / 2);
        InitMusicalEvent(&measure.events[0 + offset], DURATION_8TH, NOTE_C, octave);
        InitMusicalEvent(&measure.events[1 + offset], DURATION_16TH, NOTE_E, octave);
        AppendMusicalEvent(&measure.events[1 + offset], NOTE_G, octave);
        InitMusicalEvent(&measure.events[2 + offset], DURATION_16TH, SILENCE, octave);

        InitMusicalEvent(&measure.events[3 + offset], DURATION_8TH, NOTE_G, octave - 1);
        InitMusicalEvent(&measure.events[4 + offset], DURATION_16TH, NOTE_E, octave);
        AppendMusicalEvent(&measure.events[4 + offset], NOTE_G, octave);
        InitMusicalEvent(&measure.events[5 + offset], DURATION_16TH, SILENCE, octave);
    }

    ASSERT(GetMeasureDuration(&measure) == DURATION_WHOLE);

    return measure;
}

