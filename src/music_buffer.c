static void InitMusicBuffers() {
    sharedState->audioBackBufferIndex = DEFAULT_AUDIO_BACK_BUFFER_INDEX;
    audioThreadState->audioFrontBufferIndex = DEFAULT_AUDIO_FRONT_BUFFER_INDEX;
    state->mirrorBackBufferIndex = DEFAULT_MIRROR_BACK_BUFFER_INDEX;
    state->mirrorFrontBufferIndex = DEFAULT_MIRROR_FRONT_BUFFER_INDEX;
}

static MusicBuffer *GetAudioBackBuffer() {
    int index = sharedState->audioBackBufferIndex;
    ASSERT(index < AUDIO_BUFFER_COUNT);
    return &sharedState->audioBuffers[index];
}
static MusicBuffer *GetAudioFrontBuffer() {
    int index = audioThreadState->audioFrontBufferIndex;
    ASSERT(index < AUDIO_BUFFER_COUNT);
    return &sharedState->audioBuffers[index];
}
static MusicBuffer *GetMirrorBackBuffer() {
    int index = state->mirrorBackBufferIndex;
    ASSERT(index < AUDIO_BUFFER_COUNT);
    return &state->mirrorBuffers[index];
}
static MusicBuffer *GetMirrorFrontBuffer() {
    int index = state->mirrorFrontBufferIndex;
    ASSERT(index < AUDIO_BUFFER_COUNT);
    return &state->mirrorBuffers[index];
}

static void SwapBuffers(int *bufferIndexA, int *bufferIndexB) {
    int a = *bufferIndexA;
    int b = *bufferIndexB;
    *bufferIndexA = b;
    *bufferIndexB = a;
}

