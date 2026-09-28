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
#include "terminal.c"
#include "controller.c"
#include "song_name.c"

#ifdef DEBUG
#include "test.c"
#endif

// used before for sliding pitch and stuff, maybe revisit
// static float MoveTowards(float current, float target, float multiplier) {
//     return target + (current - target) * multiplier;
// }

void UpdateStateSong(State *state, uint64_t songIndex) {
    state->songIndex = songIndex;
    GetSongName(songIndex, &state->songName);
    sharedState->randomState = DeterministicRandom(songIndex);
}

void Update(State *state) {
    bool controllerIsPlaying = hasAllFlags(state->controller.flags, FLAG_CONTROLLER_PLAY);

    int playState = atomic_load_explicit(&sharedState->playState, memory_order_acquire);
    bool isAudioBackBufferPrepared = atomic_load_explicit(&sharedState->isAudioBackBufferPrepared, memory_order_acquire);

    bool shouldStop = false;

    bool requestNextSong = hasAllFlags(state->controller.flags, FLAG_CONTROLLER_NEXT_SONG);
    bool requestPreviousSong = hasAllFlags(state->controller.flags, FLAG_CONTROLLER_PREVIOUS_SONG);

    if (requestNextSong) {
        state->controller.flags &= ~FLAG_CONTROLLER_NEXT_SONG;

        if (state->songIndex < SONG_LAST_INDEX) {
            shouldStop = true;

            uint64_t nextSongIndex = state->songIndex + 1;
            UpdateStateSong(state, nextSongIndex);
        }
    } else if (requestPreviousSong) {
        state->controller.flags &= ~FLAG_CONTROLLER_PREVIOUS_SONG;

        if (state->songIndex > SONG_FIRST_INDEX) {
            shouldStop = true;

            uint64_t previousSongIndex = state->songIndex - 1;
            UpdateStateSong(state, previousSongIndex);
        }
    }

    switch (playState) {
        default: ASSERT(false); return;
        case PLAY_STATE_IDLE:
        {
            // before transitioning into the running state, the audio back buffer must be prepared
            if (controllerIsPlaying && isAudioBackBufferPrepared) {
                atomic_store_explicit(&sharedState->playState, PLAY_STATE_RUNNING, memory_order_release);
                state->isPlaying = true;
            }
            break;
        }
        case PLAY_STATE_RUNNING:
        {
            if (controllerIsPlaying) {
                break;
            } else {
                shouldStop = true;

                // reset random state to return to the beginning of the song
                sharedState->randomState = DeterministicRandom(state->songIndex);
                break;
            }
        }
        case PLAY_STATE_STOP_AUDIO_THREAD:
        {
            // waiting for audio thread to stop
            return;
        }
        case PLAY_STATE_STOP_MAIN_THREAD:
        {
            // here the main thread has ownership of all the
            // shared state and can safely reset all of it

            for (int i = 0; i < AUDIO_BUFFER_COUNT; i++) {
                sharedState->audioBuffers[i] = (MusicBuffer){0};
            }

            for (int i = 0; i < MIRROR_BUFFER_COUNT; i++) {
                state->mirrorBuffers[i] = (MusicBuffer){0};
            }

            atomic_store_explicit(&sharedState->isAudioBackBufferPrepared, false, memory_order_relaxed);
            atomic_store_explicit(&sharedState->playState, PLAY_STATE_IDLE, memory_order_relaxed);

            InitMusicBuffers(state);
            InitChordQueue(&state->chordQueue, GetAudioBackBuffer()->chord);

            state->isPlaying = false;

            return;
        }
    }

    if (shouldStop) {
        // stop audio thread first in order to let the main thread do full music buffer cleanup after
        atomic_store_explicit(&sharedState->playState, PLAY_STATE_STOP_AUDIO_THREAD, memory_order_release);
        return;
    }

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

        audioBackBuffer->scale = state->terminal.scale;
        audioBackBuffer->chord = GetNextChordInProgression(audioBackBuffer->scale, &state->chordQueue);
        audioBackBuffer->rhythmType = state->terminal.rhythmType;

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

    Rectangle controllerRectangle;
    {
        controllerRectangle.x = 0;
        controllerRectangle.y = GetScreenHeight() * 0.9;
        controllerRectangle.width = GetScreenWidth();
        controllerRectangle.height = GetScreenHeight() - controllerRectangle.y;
    }

    ControllerUpdate(state, &state->controller, controllerRectangle);

    Rectangle bigRectangle = {
        .x = controllerRectangle.x,
        .y = controllerRectangle.y,
        .width = GetScreenWidth(),
        .height = GetScreenHeight() - controllerRectangle.height,
    };

    GuiContainerUpdate(bigRectangle, state->guiContainers);

    SequencerUpdate(state, &state->sequencer);

    TerminalUpdate(&state->terminal);
}

void Render(const State *state) {
    BeginDrawing();

    ClearBackground((Color){0,0,0,255});

    ControllerRender(state, &state->controller, state->font);

    GuiContainerRender(state->guiContainers, state->font);
    SequencerRender(state);

    TerminalRender(&state->terminal, state->font);

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

    state->songIndex = 1;
    GetSongName(state->songIndex, &state->songName);

    sharedState->randomState = DeterministicRandom(state->songIndex);

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

    PlayAudioStream(stream);

    state->font = LoadFontEx("ComicMono.ttf", 300, NULL, 0);

    GuiContainerInitialize(state->guiContainers);
    TerminalInitialize(&state->terminal, &state->guiContainers[GUI_CONTAINER_MENU]);

    while (!WindowShouldClose()) {
        state->bpm = atomic_load_explicit(&sharedState->bpm, memory_order_relaxed);
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

