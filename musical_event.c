static Tone CreateTone(uint8_t note, uint8_t octave) {
    return (Tone) {
        .note = note,
        .octave = octave,
        .frequency = 0.0f,
        .sineIndex = 0.0f,
        .volume = 1.0f,
    };
}

static uint64_t MusicalEventDurationToSampleDuration(int duration) {
    return duration * (SAMPLE_RATE * 0.3f); // TODO: should be based on BPM
}

static MusicalEvent GenerateToneEvent() {
    MusicalEvent event = {0};
    event.toneCount = 1;

    Tone tone = CreateTone(NextRandom() % NOTE_COUNT, 4);

    switch (NextRandom() % 3) {
        case 0: event.duration = DURATION_16TH; break;
        case 1: event.duration = DURATION_8TH; break;
        case 2: event.duration = DURATION_4TH; break;
        default: ASSERT(false); break;
    }

    event.tones[0] = tone;

    return event;
}

static void InitMusicalEvent(MusicalEvent *event, uint8_t duration, uint8_t note, uint8_t octave) {
    event->duration = duration;
    event->toneCount = 1;
    event->tones[0] = (Tone){
        .octave = octave,
        .note = note,
    };
}

static void AppendMusicalEvent(MusicalEvent *event, uint8_t note, uint8_t octave) {
    ASSERT((event->toneCount + 1) < MUSICAL_EVENT_MAX_TONES);
    event->toneCount++;
    event->tones[event->toneCount - 1] = CreateTone(note, octave);
}

