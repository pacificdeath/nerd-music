// TODO: only compile on debug, measure should always be length DURATION_WHOLE
static float GetMeasureDuration(const Measure *measure) {
    ASSERT((measure->eventCount) < MEASURE_EVENT_CAPACITY);
    float duration = 0;
    for (int i = 0; i < measure->eventCount; i++) {
        duration += measure->events[i].duration;
    }
    return duration;
}

typedef struct MelodyState {
    const MusicBuffer *buffer;
    const MusicalEvent *finalEventInPreviousMeasure;
    int currentEventIndex;
    int forceDuration;
} MelodyState;

// TODO: temporary
#define MELODY_LOWEST_NOTE (4 * NOTES_PER_OCTAVE)
#define MELODY_HIGHEST_NOTE ((5 * NOTES_PER_OCTAVE))
static void GetNextMelodyEvent(const MelodyState *state, MusicalEvent *result) {
    int direction;

    const Measure *measure = &state->buffer->measures[MEASURE_MELODY];
    const int eventIndex = state->currentEventIndex;

    const bool isFirstEventInMeasure = (eventIndex == 0);
    const bool isFirstEventGlobally = isFirstEventInMeasure && (state->finalEventInPreviousMeasure == NULL);

    int note;
    int previousDuration = 0;
    int consecutiveEqualDurations = 0;

    if (isFirstEventGlobally) {
        int lowHighDiff = MELODY_HIGHEST_NOTE - MELODY_LOWEST_NOTE;
        note = MELODY_LOWEST_NOTE + (NextRandom() % lowHighDiff);
    } else if (isFirstEventInMeasure) {
        note = state->finalEventInPreviousMeasure->tones[0].note;
    } else {
        int previousEventIndex = eventIndex - 1;
        const MusicalEvent *previousEvent = &measure->events[previousEventIndex];
        note = previousEvent->tones[0].note;
        previousDuration = previousEvent->duration;
        for (int i = previousEventIndex; i >= 0; i--) {
            if (measure->events[i].duration == previousDuration) {
                consecutiveEqualDurations++;
            } else {
                break;
            }
        }
    }

    if (note < MELODY_LOWEST_NOTE) {
        direction = 1;
    } else if (note > MELODY_HIGHEST_NOTE) {
        direction = -1;
    } else {
        direction = ((NextRandom() % 2) == 0) ? 1 : -1;
    }

    int duration;
    if (state->forceDuration == 0) {
        // DURATION BASED ON NOTE

        int noteType;

        if (isFirstEventInMeasure) {
            // first note of every measure is a chord note, will sound intentional probably?
            noteType = NOTE_TYPE_CHORD;
        } else {
            noteType = NextRandom() % NOTE_TYPES_COUNT;
        }

        switch (noteType) {
            default: ASSERT(false); break;
            case NOTE_TYPE_CHORD:
                do {
                    note += direction;
                } while (!IsNoteInChord(state->buffer->chord, note));
                break;
            case NOTE_TYPE_SCALE:
                do {
                    note += direction;
                } while (!IsNoteInScale(state->buffer->scale, note));
                break;
            case NOTE_TYPE_CHROMATIC:
                note += direction;
                break;
        }

        // 16th notes must exist in pairs of 2 at least, otherwise it sounds banana
        bool force16th = (previousDuration == DURATION_16TH) && ((consecutiveEqualDurations % 2) != 0);

        // 32th notes must exist in pairs of 4 at least, otherwise it sounds banana
        bool force32th = (previousDuration == DURATION_32TH) && ((consecutiveEqualDurations % 4) != 0);

        if (force32th) {
            duration = DURATION_32TH;
        } else if (force16th) {
            duration = DURATION_16TH;
        } else {
            // random duration
            switch (noteType) {
                default: ASSERT(false); break;
                case NOTE_TYPE_CHORD:
                {
                    // only allow the DURATION_4TH to be the first event in the
                    // measure as some sort of a long target note for the new chord,
                    // they should be used sparingly as they are slow and boring,
                    // TODO: instead of doing this thing, the caller should
                    // be able to specify the longest duration allowed
                    const int choiceCount = isFirstEventInMeasure ? 4 : 3;
                    switch (NextRandom() % choiceCount) {
                        default: ASSERT(false);
                        case 0: duration = DURATION_32TH; break;
                        case 1: duration = DURATION_16TH; break;
                        case 2: duration = DURATION_8TH; break;
                        case 3: duration = DURATION_4TH; break;
                    }
                    break;
                }
                case NOTE_TYPE_SCALE:
                    switch (NextRandom() % 3) {
                        default: ASSERT(false);
                        case 0: duration = DURATION_32TH; break;
                        case 1: duration = DURATION_16TH; break;
                        case 2: duration = DURATION_8TH; break;
                    }
                    break;
                case NOTE_TYPE_CHROMATIC:
                    duration = DURATION_16TH;
                    break;
            }
        }
    } else {
        // NOTE BASED ON DURATION

        duration = state->forceDuration;
        int noteType;
        if (duration <= DURATION_16TH) {
            switch (NextRandom() % 3) {
                default: ASSERT(false);
                case 0: noteType = NOTE_TYPE_CHORD; break;
                case 1: noteType = NOTE_TYPE_SCALE; break;
                case 2: noteType = NOTE_TYPE_CHROMATIC; break;
            }
        } else if (duration <= DURATION_8TH) {
            switch (NextRandom() % 2) {
                default: ASSERT(false);
                case 0: noteType = NOTE_TYPE_CHORD; break;
                case 1: noteType = NOTE_TYPE_SCALE; break;
            }
        } else {
            noteType = NOTE_TYPE_CHORD;
        }

        switch (noteType) {
            default: ASSERT(false);
            case NOTE_TYPE_CHORD:
                do {
                    note += direction;
                } while (!IsNoteInChord(state->buffer->chord, note));
                break;
            case NOTE_TYPE_SCALE:
                do {
                    note += direction;
                } while (!IsNoteInScale(state->buffer->scale, note));
                break;
            case NOTE_TYPE_CHROMATIC:
                note += direction;
                break;
        }
    }

    Tone tone = CreateTone(note);

    InitMusicalEvent(result, tone, duration);
}

static void GenerateMeasureWithRepeatingRhythms(
    const MusicBuffer *currentBuffer,
    const MusicBuffer *previousBuffer,
    Measure *measure,
    int size
) {
    *measure = (Measure){0};

    const int times = DURATION_WHOLE / size;
    ASSERT(times > 0);

    int duration = 0;
    int eventsPerSize;
    const Measure *oldMeasure = &previousBuffer->measures[MEASURE_MELODY];

    MelodyState melodyState = {0};
    melodyState.buffer = currentBuffer;
    if (oldMeasure->eventCount > 0) {
        melodyState.finalEventInPreviousMeasure = &oldMeasure->events[oldMeasure->eventCount - 1];
    } else {
        melodyState.finalEventInPreviousMeasure = NULL;
    }
    melodyState.forceDuration = 0;

    for (eventsPerSize = 0; eventsPerSize < size; eventsPerSize++) {
        melodyState.currentEventIndex = eventsPerSize;
        MusicalEvent *currentEvent = &measure->events[eventsPerSize];

        GetNextMelodyEvent(&melodyState, currentEvent);

        duration += currentEvent->duration;
        if (duration >= size) {
            int overflow = (duration - size);
            currentEvent->duration -= overflow;
            duration -= overflow;
            eventsPerSize++;
            break;
        }
    }

    ASSERT((duration * times) == DURATION_WHOLE);

    for (int timeIndex = 1; timeIndex < times; timeIndex++) {
        for (int eventIndex = 0; eventIndex < eventsPerSize; eventIndex++) {
            // this is an event-duration in the first "size" that will match the
            // duration of events in other "sizes" with the same event index,
            // this is to create "rhythmic motifs"
            melodyState.forceDuration = measure->events[eventIndex].duration;
            melodyState.currentEventIndex = (timeIndex * eventsPerSize) + eventIndex;

            MusicalEvent *currentEvent = &measure->events[melodyState.currentEventIndex];
            GetNextMelodyEvent(&melodyState, currentEvent);
        }
    }

    measure->eventCount = eventsPerSize * times;
}

static void GenerateMelodyMeasure(
    const MusicBuffer *currentBuffer,
    const MusicBuffer *previousBuffer,
    Measure *measure
) {
    int duration = 0;
    switch (NextRandom() % 3) {
        default: ASSERT(false); return;

        // full random
        case 0: duration = DURATION_WHOLE; break;

        // rythmic motif x 2
        case 1: duration = DURATION_HALF; break;

        // rythmic motif x 4
        case 2: duration = DURATION_4TH; break;
    }

    GenerateMeasureWithRepeatingRhythms(currentBuffer, previousBuffer, measure, duration);
}

// TODO: temporary
#define HARMONY_LOWEST_NOTE (3 * NOTES_PER_OCTAVE)
static void GenerateHarmonyMeasure(const MusicBuffer *buffer, Measure *measure) {
    bool allow7thBase = false;
    ChordInversion inversion = CreateLowChordInversion(buffer->chord, HARMONY_LOWEST_NOTE, allow7thBase);

    measure->eventCount = 24;
    const int reps = 2;

    for (int i = 0; i < reps; i++) {
        int offset = i * (measure->eventCount / reps);
        InitMusicalEvent(&measure->events[0 + offset], CreateTone(inversion.notes[0]), DURATION_8TH);
        InitMusicalEvent(&measure->events[1 + offset], CreateTone(inversion.notes[1]), DURATION_16TH);
        AppendMusicalEvent(&measure->events[1 + offset], CreateTone(inversion.notes[2]));
        InitMusicalEvent(&measure->events[2 + offset], CreateTone(SILENCE), DURATION_16TH);

        InitMusicalEvent(&measure->events[3 + offset], CreateTone(inversion.notes[2] - NOTES_PER_OCTAVE), DURATION_8TH);
        InitMusicalEvent(&measure->events[4 + offset], CreateTone(inversion.notes[1]), DURATION_16TH);
        AppendMusicalEvent(&measure->events[4 + offset], CreateTone(inversion.notes[2]));
        InitMusicalEvent(&measure->events[5 + offset], CreateTone(SILENCE), DURATION_16TH);
    }

    ASSERT(GetMeasureDuration(measure) == DURATION_WHOLE);
}

