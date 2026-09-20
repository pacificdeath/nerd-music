#include "main.h"

#include "random.c"
#include "tone.c"
#include "chord.c"
#include "scale.c"
#include "music_buffer.c"
#include "musical_event.c"
#include "measure.c"

#include "audio_thread.c"

// visuals
#include "gui_container.c"
#include "sequencer.c"
#include "menu.c"

#ifdef DEBUG
#include "test.c"
#endif

// used before for sliding pitch and stuff, maybe revisit
// static float MoveTowards(float current, float target, float multiplier) {
//     return target + (current - target) * multiplier;
// }

void Update(State *state) {
    bool isAudioBackBufferPrepared = atomic_load_explicit(&sharedState->isAudioBackBufferPrepared, memory_order_acquire);

    if (!isAudioBackBufferPrepared) {
        {
            // old mirror buffer is never swapped so it always has the default index
            MusicBuffer *mirrorOldBuffer = GetMirrorBuffer(state->mirrorBuffers, DEFAULT_MIRROR_OLD_BUFFER_INDEX);
            // copy the now completed front buffer into the old mirror buffer
            MusicBuffer *mirrorFrontBuffer = GetMirrorBuffer(state->mirrorBuffers, state->mirrorFrontBufferIndex);
            *mirrorOldBuffer = *mirrorFrontBuffer;
        }

        // the audio thread has swapped the audio front and back buffers,
        // this means we have to swap the mirror buffers as well
        SwapBuffers(
            &state->mirrorBackBufferIndex,
            &state->mirrorFrontBufferIndex
        );

        // regenerate into audio back buffer
        MusicBuffer *audioBackBuffer = GetAudioBackBuffer();
        audioBackBuffer->scale = state->menu.scale;
        audioBackBuffer->chord = GetNextChordInProgression(audioBackBuffer->scale, &state->chordQueue);
        audioBackBuffer->rhythmType = state->menu.rhythmType;

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

    GuiContainerUpdate(state->guiContainers);

    MenuUpdate(&state->menu);
}

void Render(const State *state) {
    BeginDrawing();

    ClearBackground((Color){0,0,0,255});

    GuiContainerRender(state->guiContainers, state->font);
    SequencerRender(state);

    MenuRender(&state->menu, state->font);

    EndDrawing();
}

int main() {
    // smallest 4/4
    ASSERT((MEASURE_EVENT_CAPACITY % 32) == 0);
    // smallest 3/4
    ASSERT((MEASURE_EVENT_CAPACITY % 24) == 0);

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

    // TODO: bpm should be configurable at runtime
    sharedState->bpm = 120.0f;

    InitMusicBuffers(state);
    InitChordQueue(&state->chordQueue, GetAudioBackBuffer()->chord);

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1600, 1200, "The music program");
    SetTargetFPS(60);

    InitAudioDevice();

    SetAudioStreamBufferSizeDefault(AUDIO_STREAM_SIZE);
    AudioStream stream = LoadAudioStream(SAMPLE_RATE, SAMPLE_SIZE, CHANNELS);
    SetAudioStreamCallback(stream, AudioInputCallback);

    sharedState->randomState = 90; // TODO

    PlayAudioStream(stream);

    state->font = LoadFontEx("ComicMono.ttf", 300, NULL, 0);

    GuiContainerInitialize(state->guiContainers);
    MenuInitialize(&state->menu, &state->guiContainers[GUI_CONTAINER_MENU]);

    while (!WindowShouldClose()) {
        Update(state);
        Render(state);
    }

    UnloadFont(state->font);

    StopAudioStream(stream);
    UnloadAudioStream(stream);

    CloseAudioDevice();
    CloseWindow();

    free(state);
    free(sharedState);
    free(audioThreadState);
}

