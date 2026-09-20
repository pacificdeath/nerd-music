#ifdef DEBUG
// only used to assert measure duration, measures should always be length DURATION_WHOLE
static float GetMeasureDuration(const Measure *measure) {
    ASSERT((measure->eventCount) < MEASURE_EVENT_CAPACITY);
    float duration = 0;
    for (int i = 0; i < measure->eventCount; i++) {
        duration += measure->events[i].duration;
    }
    return duration;
}
#endif

static bool IsTripleMeter(int rhythmType) {
    switch (rhythmType) {
        default: ASSERT(false);
        case RHYTHM_TYPE_OOMPHA:
        case RHYTHM_TYPE_ARPEGGIO:
        case RHYTHM_TYPE_FAST_ARPEGGIO:
            return false;
        case RHYTHM_TYPE_OOMPHA_TRIPLET:
        case RHYTHM_TYPE_WALTZ:
        case RHYTHM_TYPE_ARPEGGIO_TRIPLET:
        case RHYTHM_TYPE_FAST_ARPEGGIO_TRIPLET:
            return true;
    }
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
static void GetNextMelodyEvent(const MelodyState *state, MusicalEvent *result, bool isTripleMeter) {
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
            WeightedIdList list = {0};
            AppendWeightedId(&list, (WeightedId){ .id = NOTE_TYPE_CHORD, .weight = MELODY_CHORD_NOTE_WEIGHT });
            AppendWeightedId(&list, (WeightedId){ .id = NOTE_TYPE_SCALE, .weight = MELODY_SCALE_NOTE_WEIGHT });
            AppendWeightedId(&list, (WeightedId){ .id = NOTE_TYPE_CHROMATIC, .weight = MELODY_CHROMATIC_NOTE_WEIGHT });
            WeightedId item = GetRandomIdByWeight(&list);
            noteType = item.id;
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
        bool force16th = (previousDuration == DURATION_16) && ((consecutiveEqualDurations % 2) != 0);

        // 32th notes must exist in pairs of 4 at least, otherwise it sounds banana
        bool force32th = (previousDuration == DURATION_32) && ((consecutiveEqualDurations % 4) != 0);

        bool force24th = (previousDuration == DURATION_24) && ((consecutiveEqualDurations % 2) != 0);

        if (force32th) {
            duration = DURATION_32;
        } else if (force16th) {
            duration = DURATION_16;
        } else if (force24th) {
            duration = DURATION_24;
        } else {
            // random duration
            switch (noteType) {
                default: ASSERT(false); break;
                case NOTE_TYPE_CHORD:
                {
                    // only allow the DURATION_4TH to be the first event in the
                    // measure as some sort of a long target note for the new chord,
                    // they should be used sparingly as they are slow and boring,
                    // TODO: instead of doing this thing, the caller should be able to specify the longest duration allowed
                    const int choiceCount = isFirstEventInMeasure ? 4 : 3;
                    if (isTripleMeter) {
                        switch (NextRandom() % choiceCount) {
                            default: ASSERT(false);
                            case 0: duration = DURATION_24; break;
                            case 1: duration = DURATION_12; break;
                            case 2: duration = DURATION_6; break;
                            case 3: duration = DURATION_4; break;
                        }
                    } else {
                        switch (NextRandom() % choiceCount) {
                            default: ASSERT(false);
                            case 0: duration = DURATION_32; break;
                            case 1: duration = DURATION_16; break;
                            case 2: duration = DURATION_8; break;
                            case 3: duration = DURATION_4; break;
                        }
                    }
                    break;
                }
                case NOTE_TYPE_SCALE:
                    if (isTripleMeter) {
                        switch (NextRandom() % 3) {
                            default: ASSERT(false);
                            case 0: duration = DURATION_24; break;
                            case 1: duration = DURATION_12; break;
                            case 2: duration = DURATION_6; break;
                        }
                    } else {
                        switch (NextRandom() % 3) {
                            default: ASSERT(false);
                            case 0: duration = DURATION_32; break;
                            case 1: duration = DURATION_16; break;
                            case 2: duration = DURATION_8; break;
                        }
                    }
                    break;
                case NOTE_TYPE_CHROMATIC:
                    if (isTripleMeter) {
                        duration = DURATION_24;
                    } else {
                        duration = DURATION_16;
                    }
                    break;
            }
        }
    } else {
        // NOTE BASED ON DURATION

        duration = state->forceDuration;
        int noteType;

        int chromaticNoteThreshold = isTripleMeter
            ? DURATION_24
            : DURATION_16;

        int scaleNoteThreshold = isTripleMeter
            ? DURATION_12
            : DURATION_8;

        if (duration <= chromaticNoteThreshold) {
            switch (NextRandom() % 3) {
                default: ASSERT(false);
                case 0: noteType = NOTE_TYPE_CHORD; break;
                case 1: noteType = NOTE_TYPE_SCALE; break;
                case 2: noteType = NOTE_TYPE_CHROMATIC; break;
            }
        } else if (duration <= scaleNoteThreshold) {
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
    bool isTripleMeter = IsTripleMeter(currentBuffer->rhythmType);

    ASSERT(size > 0);

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

        GetNextMelodyEvent(&melodyState, currentEvent, isTripleMeter);

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
            GetNextMelodyEvent(&melodyState, currentEvent, isTripleMeter);
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
        case 2: duration = DURATION_4; break;
    }

    GenerateMeasureWithRepeatingRhythms(currentBuffer, previousBuffer, measure, duration);
}

// TODO: temporary
#define HARMONY_LOWEST_NOTE (3 * NOTES_PER_OCTAVE)

static void GenerateOompahHarmony(const MusicBuffer *buffer, Measure *measure) {
    bool allow7thBase = false;
    ChordInversion inversion = CreateLowChordInversion(buffer->chord, HARMONY_LOWEST_NOTE, allow7thBase);

    int index = 0;

    for (int i = 0; i < 2; i++) {
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[0]), DURATION_8);
        InitMusicalEvent(&measure->events[index], CreateTone(inversion.notes[1]), DURATION_16);
        AppendMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2]));
        InitMusicalEvent(&measure->events[index++], CreateTone(SILENCE), DURATION_16);

        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2] - NOTES_PER_OCTAVE), DURATION_8);
        InitMusicalEvent(&measure->events[index], CreateTone(inversion.notes[1]), DURATION_16);
        AppendMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2]));
        InitMusicalEvent(&measure->events[index++], CreateTone(SILENCE), DURATION_16);
    }

    measure->eventCount = index;
}

static void GenerateOompahX3Harmony(const MusicBuffer *buffer, Measure *measure) {
    bool allow7thBase = false;
    ChordInversion inversion = CreateLowChordInversion(buffer->chord, HARMONY_LOWEST_NOTE, allow7thBase);

    int index = 0;

    for (int i = 0; i < 2; i++) {
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[0]), DURATION_12);
        InitMusicalEvent(&measure->events[index++], CreateTone(SILENCE), DURATION_12);
        InitMusicalEvent(&measure->events[index], CreateTone(inversion.notes[1]), DURATION_12);
        AppendMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2]));

        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2] - NOTES_PER_OCTAVE), DURATION_12);
        InitMusicalEvent(&measure->events[index++], CreateTone(SILENCE), DURATION_12);
        InitMusicalEvent(&measure->events[index], CreateTone(inversion.notes[1]), DURATION_12);
        AppendMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2]));
    }

    measure->eventCount = index;
}

static void GenerateWaltzHarmony(const MusicBuffer *buffer, Measure *measure) {
    bool allow7thBase = false;
    ChordInversion inversion = CreateLowChordInversion(buffer->chord, HARMONY_LOWEST_NOTE, allow7thBase);

    int index = 0;

    for (int i = 0; i < 2; i++) {
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[0]), DURATION_12);

        for (int j = 0; j < 2; j++) {
            InitMusicalEvent(&measure->events[index], CreateTone(inversion.notes[1]), DURATION_24);
            AppendMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2]));
            InitMusicalEvent(&measure->events[index++], CreateTone(SILENCE), DURATION_24);
        }

        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2] - NOTES_PER_OCTAVE), DURATION_12);

        for (int j = 0; j < 2; j++) {
            InitMusicalEvent(&measure->events[index], CreateTone(inversion.notes[1]), DURATION_24);
            AppendMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2]));
            InitMusicalEvent(&measure->events[index++], CreateTone(SILENCE), DURATION_24);
        }
    }

    measure->eventCount = index;
}

static void GenerateArpeggioHarmony(const MusicBuffer *buffer, Measure *measure) {
    bool allow7thBase = true;
    ChordInversion inversion = CreateLowChordInversion(buffer->chord, HARMONY_LOWEST_NOTE, allow7thBase);

    int index = 0;

    for (int i = 0; i < 2; i++) {
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[0]), DURATION_16);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[1]), DURATION_16);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2]), DURATION_16);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[3]), DURATION_16);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[0] + NOTES_PER_OCTAVE), DURATION_16);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[3]), DURATION_16);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2]), DURATION_16);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[1]), DURATION_16);
    }

    measure->eventCount = index;
}

static void GenerateFastArpeggioHarmony(const MusicBuffer *buffer, Measure *measure) {
    bool allow7thBase = true;
    ChordInversion inversion = CreateLowChordInversion(buffer->chord, HARMONY_LOWEST_NOTE, allow7thBase);

    int index = 0;

    for (int i = 0; i < 2; i++) {
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[0]), DURATION_32);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[1]), DURATION_32);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2]), DURATION_32);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[3]), DURATION_32);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[0] + NOTES_PER_OCTAVE), DURATION_32);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[1] + NOTES_PER_OCTAVE), DURATION_32);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2] + NOTES_PER_OCTAVE), DURATION_32);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[3] + NOTES_PER_OCTAVE), DURATION_32);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[0] + (2*NOTES_PER_OCTAVE)), DURATION_32);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[3] + NOTES_PER_OCTAVE), DURATION_32);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2] + NOTES_PER_OCTAVE), DURATION_32);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[1] + NOTES_PER_OCTAVE), DURATION_32);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[0] + NOTES_PER_OCTAVE), DURATION_32);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[3]), DURATION_32);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2]), DURATION_32);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[1]), DURATION_32);
    }

    measure->eventCount = index;
}

static void GenerateArpeggioTripletHarmony(const MusicBuffer *buffer, Measure *measure) {
    bool allow7thBase = true;
    ChordInversion inversion = CreateLowChordInversion(buffer->chord, HARMONY_LOWEST_NOTE, allow7thBase);

    int index = 0;

    for (int i = 0; i < 2; i++) {
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[0]), DURATION_12);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[1]), DURATION_12);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2]), DURATION_12);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[3]), DURATION_12);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2]), DURATION_12);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[1]), DURATION_12);
    }

    measure->eventCount = index;
}

static void GenerateFastArpeggioTripletHarmony(const MusicBuffer *buffer, Measure *measure) {
    bool allow7thBase = true;
    ChordInversion inversion = CreateLowChordInversion(buffer->chord, HARMONY_LOWEST_NOTE, allow7thBase);

    int index = 0;

    for (int i = 0; i < 2; i++) {
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[0]), DURATION_24);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[1]), DURATION_24);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2]), DURATION_24);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[3]), DURATION_24);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[0] + NOTES_PER_OCTAVE), DURATION_24);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[1] + NOTES_PER_OCTAVE), DURATION_24);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2] + NOTES_PER_OCTAVE), DURATION_24);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[1] + NOTES_PER_OCTAVE), DURATION_24);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[0] + NOTES_PER_OCTAVE), DURATION_24);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[3]), DURATION_24);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[2]), DURATION_24);
        InitMusicalEvent(&measure->events[index++], CreateTone(inversion.notes[1]), DURATION_24);
    }

    measure->eventCount = index;
}

static void GenerateHarmonyMeasure(const MusicBuffer *buffer, Measure *measure) {
    switch (buffer->rhythmType) {
        case RHYTHM_TYPE_OOMPHA:                    GenerateOompahHarmony(buffer, measure); break;
        case RHYTHM_TYPE_OOMPHA_TRIPLET:            GenerateOompahX3Harmony(buffer, measure); break;
        case RHYTHM_TYPE_WALTZ:                     GenerateWaltzHarmony(buffer, measure); break;
        case RHYTHM_TYPE_ARPEGGIO:                  GenerateArpeggioHarmony(buffer, measure); break;
        case RHYTHM_TYPE_FAST_ARPEGGIO:             GenerateFastArpeggioHarmony(buffer, measure); break;
        case RHYTHM_TYPE_ARPEGGIO_TRIPLET:          GenerateArpeggioTripletHarmony(buffer, measure); break;
        case RHYTHM_TYPE_FAST_ARPEGGIO_TRIPLET:     GenerateFastArpeggioTripletHarmony(buffer, measure); break;
    }

    ASSERT(GetMeasureDuration(measure) == DURATION_WHOLE);
}

