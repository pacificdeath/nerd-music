static void InitMusicBuffers(State *state) {
    sharedState->audioBackBufferIndex = DEFAULT_AUDIO_BACK_BUFFER_INDEX;
    sharedState->audioFrontBufferIndex = DEFAULT_AUDIO_FRONT_BUFFER_INDEX;

    state->mirrorBackBufferIndex = DEFAULT_MIRROR_BACK_BUFFER_INDEX;
    state->mirrorFrontBufferIndex = DEFAULT_MIRROR_FRONT_BUFFER_INDEX;

    // TODO: temporary scales and chords:
    Scale scale = CreateScaleFromType(NOTE_C, SCALE_LOCRIAN);
    Chord chord = CreateChordFromScaleDegree(scale, 0);

    MusicBuffer *audioBackBuffer = &sharedState->audioBuffers[sharedState->audioBackBufferIndex];
    audioBackBuffer->scale = scale;
    audioBackBuffer->chord = chord;

    MusicBuffer *audioFrontBuffer = &sharedState->audioBuffers[sharedState->audioFrontBufferIndex];
    audioFrontBuffer->scale = scale;
    audioFrontBuffer->chord = chord;
}

static MusicBuffer *GetAudioBackBuffer() {
    int index = sharedState->audioBackBufferIndex;
    ASSERT(index < AUDIO_BUFFER_COUNT);
    return &sharedState->audioBuffers[index];
}
static MusicBuffer *GetAudioFrontBuffer() {
    int index = sharedState->audioFrontBufferIndex;
    ASSERT(index < AUDIO_BUFFER_COUNT);
    return &sharedState->audioBuffers[index];
}
static MusicBuffer *GetMirrorBuffer(MusicBuffer mirrorBuffers[MIRROR_BUFFER_COUNT], int bufferIndex) {
    ASSERT(bufferIndex < AUDIO_BUFFER_COUNT);
    return &mirrorBuffers[bufferIndex];
}
static const MusicBuffer *GetReadonlyMirrorBuffer(const MusicBuffer mirrorBuffers[MIRROR_BUFFER_COUNT], int bufferIndex) {
    ASSERT(bufferIndex < AUDIO_BUFFER_COUNT);
    return &mirrorBuffers[bufferIndex];
}

static void SwapBuffers(int *bufferIndexA, int *bufferIndexB) {
    int a = *bufferIndexA;
    int b = *bufferIndexB;
    *bufferIndexA = b;
    *bufferIndexB = a;
}

