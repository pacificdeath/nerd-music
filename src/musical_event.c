static unsigned int MusicalEventDurationToSampleDuration(int duration) {
    float quarterNoteSamples = SAMPLE_RATE * (60.0f / sharedState->bpm);
    return quarterNoteSamples * ((float)duration / (float)DURATION_4TH);
}

static void InitMusicalEvent(MusicalEvent *event, Tone tone, uint8_t duration) {
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

