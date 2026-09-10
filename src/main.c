#include "main.h"

#include "tone.c"
#include "chord.c"
#include "scale.c"
#include "music_buffer.c"
#include "musical_event.c"
#include "measure.c"
#include "sequencer.c"
#include "audio_thread.c"
#include "menu.c"

#ifdef DEBUG
#include "test.c"
#endif

// used before for sliding pitch and stuff, maybe revisit
// static float MoveTowards(float current, float target, float multiplier) {
//     return target + (current - target) * multiplier;
// }

void Update(State *state) {
    bool ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    if (ctrlDown) {
        if (IsKeyPressed(KEY_ONE)) {
            state->viewFlags ^= VIEW_FLAG_MELODY;
        }
        if (IsKeyPressed(KEY_TWO)) {
            state->viewFlags ^= VIEW_FLAG_HARMONY;
        }
        if (IsKeyPressed(KEY_THREE)) {
            state->viewFlags ^= VIEW_FLAG_MENU;
        }
    }

    bool isAudioBackBufferPrepared = atomic_load_explicit(&sharedState->isAudioBackBufferPrepared, memory_order_acquire);

    if (!isAudioBackBufferPrepared) {
        // the audio thread has swapped the audio front and back buffers,
        // this means we have to swap the mirror buffers as well
        SwapBuffers(
            &state->mirrorBackBufferIndex,
            &state->mirrorFrontBufferIndex
        );

        // regenerate into audio back buffer
        MusicBuffer *audioBackBuffer = GetAudioBackBuffer();
        audioBackBuffer->scale = state->menu.scale;
        audioBackBuffer->chord = GetNextChordInProgression(audioBackBuffer->scale, audioBackBuffer->chord);

        for (int measureIndex = 0; measureIndex < MEASURE_TOTAL; measureIndex++) {
            Measure *measure = &audioBackBuffer->measures[measureIndex];
            switch (measureIndex) {
                case MEASURE_MELODY:
                    const MusicBuffer *currentBuffer = audioBackBuffer;
                    const MusicBuffer *previousBuffer = GetMirrorBuffer(state->mirrorBuffers, state->mirrorFrontBufferIndex);
                    GenerateMelodyMeasure(currentBuffer, previousBuffer, measure);
                    break;
                case MEASURE_HARMONY:
                    GenerateHarmonyMeasure(audioBackBuffer, measure);
                    break;
            }
        }

        // copy back buffer into mirror
        MusicBuffer *mirrorBackBuffer = GetMirrorBuffer(state->mirrorBuffers, state->mirrorBackBufferIndex);
        *mirrorBackBuffer = *audioBackBuffer;

        // signal that the audio thread can start using this thing
        atomic_store_explicit(&sharedState->isAudioBackBufferPrepared, true, memory_order_release);
    }

    const bool hasMelodyView = hasFlag(state->viewFlags, VIEW_FLAG_MELODY);
    const bool hasHarmonyView = hasFlag(state->viewFlags, VIEW_FLAG_HARMONY);
    const bool hasMenuView = hasFlag(state->viewFlags, VIEW_FLAG_MENU);

    const int visibleViews = (hasMelodyView ? 1 : 0) + (hasHarmonyView ? 1 : 0) + (hasMenuView ? 1 : 0);

    state->viewHeight = (visibleViews > 0) ? (GetScreenHeight() / visibleViews) : 0.0f;

    Rectangle menuOuterRectangle = {
        .x = 0,
        .y = (hasMelodyView ? state->viewHeight : 0.0f) + (hasHarmonyView ? state->viewHeight : 0.0f),
        .width = GetScreenWidth(),
        .height = state->viewHeight,
    };

    MenuUpdate(&state->menu, menuOuterRectangle);
}

void Render(const State *state) {
    BeginDrawing();

    ClearBackground((Color){0,0,0,255});

    SequencerRender(state);
    MenuRender(&state->menu);

    EndDrawing();
}

int main() {
    State *state;

    state = (State *)calloc(sizeof(State), 1);
    ASSERT(state != NULL);

    sharedState = (SharedState *)calloc(sizeof(SharedState), 1);
    ASSERT(sharedState != NULL);

    audioThreadState = (AudioThreadState *)calloc(sizeof(AudioThreadState), 1);
    ASSERT(audioThreadState != NULL);

#ifdef DEBUG
    RunTests();
#endif

    sharedState->bpm = 180.0f;

    InitMusicBuffers(state);

    state->viewFlags = DEFAULT_VIEW_FLAGS;

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(600, 600, "The music program");
    SetTargetFPS(60);

    InitAudioDevice();

    SetAudioStreamBufferSizeDefault(AUDIO_STREAM_SIZE);
    AudioStream stream = LoadAudioStream(SAMPLE_RATE, SAMPLE_SIZE, CHANNELS);
    SetAudioStreamCallback(stream, AudioInputCallback);

    sharedState->randomState = 90; // TODO

    PlayAudioStream(stream);

    MenuInitialize(&state->menu);

    while (!WindowShouldClose()) {
        Update(state);
        Render(state);
    }

    MenuDeinitialize(&state->menu);

    StopAudioStream(stream);
    UnloadAudioStream(stream);

    CloseAudioDevice();
    CloseWindow();

    free(state);
    free(sharedState);
    free(audioThreadState);
}

