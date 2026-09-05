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

#define MELODY_LOWEST_NOTE (3 * NOTES_PER_OCTAVE)
#define MELODY_HIGHEST_NOTE (5 * NOTES_PER_OCTAVE)
static void GetNextMelodyEvent(const MusicBuffer *buffer, MusicalEvent *previous, MusicalEvent *result, uint8_t predefinedDuration) {
    int direction;
    // TODO: somehow get last tone of previous measure

    int note = (previous == NULL) ? (4 * NOTES_PER_OCTAVE) : previous->tones[0].note;

    if (note < MELODY_LOWEST_NOTE) {
        direction = 1;
    } else if (note > MELODY_HIGHEST_NOTE) {
        direction = -1;
    } else {
        direction = ((NextRandom() % 2) == 0) ? 1 : -1;
    }

    int duration;
    if (predefinedDuration == 0) {
        // duration based on note
        switch (NextRandom() % 3) {
            default: ASSERT(false); break;
            case 0: // chord note
                do {
                    note += direction;
                } while (!IsNoteInChord(buffer->chord, note));
                switch (NextRandom() % 3) {
                    default: ASSERT(false);
                    case 0: duration = DURATION_16TH; break;
                    case 1: duration = DURATION_8TH; break;
                    case 2: duration = DURATION_4TH; break;
                }
                break;
            case 1: // scale tone
                do {
                    note += direction;
                } while (!IsNoteInScale(buffer->scale, note));
                switch (NextRandom() % 2) {
                    default: ASSERT(false);
                    case 0: duration = DURATION_16TH; break;
                    case 1: duration = DURATION_8TH; break;
                }
                break;
            case 2: // chromatic tone
                note += direction;
                duration = DURATION_16TH;
                break;
        }
    } else {
        // note based on duration
        duration = predefinedDuration;
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
                } while (!IsNoteInChord(buffer->chord, note));
                break;
            case NOTE_TYPE_SCALE:
                do {
                    note += direction;
                } while (!IsNoteInScale(buffer->scale, note));
                break;
            case NOTE_TYPE_CHROMATIC:
                note += direction;
                break;
        }
    }

    Tone tone = CreateTone(note);

    InitMusicalEvent(result, tone, duration);
}

static void GenerateMeasureWithRepeatingRhythms(const MusicBuffer *buffer, Measure *measure, int size, int times) {
    *measure = (Measure){0};

    int duration = 0;
    int eventsPerSize;
    MusicalEvent *previousEvent = NULL;
    for (eventsPerSize = 0; eventsPerSize < size; eventsPerSize++) {
        MusicalEvent *currentEvent = &measure->events[eventsPerSize];
        int noPredefinedDuration = 0;
        GetNextMelodyEvent(buffer, previousEvent, currentEvent, noPredefinedDuration);
        previousEvent = currentEvent;

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

    for (int eventIndex = 0; eventIndex < eventsPerSize; eventIndex++) {
        // this is an event-duration in the first "size" that will match the
        // duration of events in other "sizes" with the same event index,
        // this is to create "rhythmic motifs"
        int masterDuration = measure->events[eventIndex].duration;
        for (int timeIndex = 1; timeIndex < times; timeIndex++) {
            int offsetEventIndex = (timeIndex * eventsPerSize) + eventIndex;
            MusicalEvent *currentEvent = &measure->events[offsetEventIndex];
            GetNextMelodyEvent(buffer, previousEvent, currentEvent, masterDuration);
            previousEvent = currentEvent;
        }
    }

    measure->eventCount = eventsPerSize * times;
}

static void GenerateMelodyMeasure(const MusicBuffer *buffer, Measure *measure) {
    switch (NextRandom() % 3) {
        case 0: // full random
            GenerateMeasureWithRepeatingRhythms(buffer, measure, DURATION_WHOLE, 1);
            return;
        case 1: // rythmic motif x 2
            GenerateMeasureWithRepeatingRhythms(buffer, measure, DURATION_HALF, 2);
            return;
        case 2: // rythmic motif x 4
            GenerateMeasureWithRepeatingRhythms(buffer, measure, DURATION_4TH, 4);
            return;
        default:
            ASSERT(false);
            return;
    }
}

static void GenerateHarmonyMeasure(Measure *measure) {
    measure->eventCount = 24;
    const int octave = 3;
    const int reps = 2;
    for (int i = 0; i < reps; i++) {
        int offset = i * (measure->eventCount / reps);
        InitMusicalEvent(&measure->events[0 + offset], CreateToneWithOctave(NOTE_C, octave), DURATION_8TH);
        InitMusicalEvent(&measure->events[1 + offset], CreateToneWithOctave(NOTE_E, octave), DURATION_16TH);
        AppendMusicalEvent(&measure->events[1 + offset], CreateToneWithOctave(NOTE_G, octave));
        InitMusicalEvent(&measure->events[2 + offset], CreateTone(SILENCE), DURATION_16TH);

        InitMusicalEvent(&measure->events[3 + offset], CreateToneWithOctave(NOTE_G, octave - 1), DURATION_8TH);
        InitMusicalEvent(&measure->events[4 + offset], CreateToneWithOctave(NOTE_E, octave), DURATION_16TH);
        AppendMusicalEvent(&measure->events[4 + offset], CreateToneWithOctave(NOTE_G, octave));
        InitMusicalEvent(&measure->events[5 + offset], CreateTone(SILENCE), DURATION_16TH);
    }

    ASSERT(GetMeasureDuration(measure) == DURATION_WHOLE);
}

