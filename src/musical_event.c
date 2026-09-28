static float MusicalEventDuration(const MusicalEvent *event, float bpm) {
    const float measureDuration = 240.0f / bpm;
    return measureDuration * event->duration / MEASURE_EVENT_CAPACITY;
}

static unsigned int MusicalEventDurationToSampleDuration(int duration, float bpm) {
    float quarterNoteSamples = SAMPLE_RATE * (60.0f / bpm);
    return (unsigned int)roundf(
        quarterNoteSamples * ((float)duration / (float)DURATION_4)
    );
}

static void InitMusicalEvent(MusicalEvent *event, Tone tone, int duration) {
    *event = (MusicalEvent){0};

    event->duration = duration;
    if (tone.note == SILENCE) {
        event->toneCount = 0;
        return;
    }

    event->toneCount = 1;
    event->tones[0] = tone;
}

static void AppendMusicalEvent(MusicalEvent *event, Tone tone) {
    ASSERT((event->toneCount + 1) < MUSICAL_EVENT_MAX_TONES);
    event->toneCount++;
    event->tones[event->toneCount - 1] = tone;
}

