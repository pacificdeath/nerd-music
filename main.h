#include <stdlib.h>
#include <math.h>
#include <stdatomic.h>

#include <stdint.h>

#include "raylib.h"

// TODO: these debug things should not always be compiled
#define ASSERT(condition)\
    do { if (!(condition)) {\
        TraceLog(LOG_ERROR, "[you are a horrible person] %s:%i -> (%s)", __FILE__, __LINE__, #condition);\
        exit(1);\
    } } while (0)

// defines:

#define MIN(a, b) ((a)<(b)?(a):(b))

#define AUDIO_STREAM_SIZE 4096
#define SAMPLE_RATE 44100
#define SAMPLE_SIZE 16
#define CHANNELS 1

// TODO: later this might be configurable?
#define EVENT_FADE_SAMPLES (SAMPLE_RATE * 0.05f)

#define DURATION_16TH (MEASURE_EVENT_CAPACITY / 16)
#define DURATION_8TH (MEASURE_EVENT_CAPACITY / 8)
#define DURATION_4TH (MEASURE_EVENT_CAPACITY / 4)
#define DURATION_HALF (MEASURE_EVENT_CAPACITY / 2)
#define DURATION_WHOLE (MEASURE_EVENT_CAPACITY)

#define SCALE_CAPACITY 7
#define SCALE_MAJOR             {0, 2, 4, 5, 7, 9, 11}
#define SCALE_DORIAN            {0, 2, 3, 5, 7, 9, 10}
#define SCALE_PHRYGIAN          {0, 1, 3, 5, 7, 8, 10}
#define SCALE_LYDIAN            {0, 2, 4, 6, 7, 9, 11}
#define SCALE_MIXOLYDIAN        {0, 2, 4, 5, 7, 9, 10}
#define SCALE_MINOR             {0, 2, 3, 5, 7, 8, 10}
#define SCALE_LOCRIAN           {0, 1, 3, 5, 6, 8, 10}
#define SCALE_HARMONIC_MINOR    {0, 2, 3, 5, 7, 8, 11}
#define SCALE_MELODIC_MINOR     {0, 2, 3, 5, 7, 9, 11}

#define SEQUENCER_LOWEST_OCTAVE 2
#define SEQUENCER_HIGHEST_OCTAVE 6
#define SEQUENCER_OCTAVE_COUNT (SEQUENCER_HIGHEST_OCTAVE - SEQUENCER_LOWEST_OCTAVE + 1)


#define MUSICAL_EVENT_MAX_TONES 4

#define MEASURE_EVENT_CAPACITY 16

#define HAS_FLAG(flags,flag) ((flags&flag)==flag)
#define FLAG_NONE (0)

#define MEASURE_FLAG_MUTED (1 << 0)

// enums:

enum {
    DEFAULT_AUDIO_BACK_BUFFER_INDEX,
    DEFAULT_AUDIO_FRONT_BUFFER_INDEX,
    AUDIO_BUFFER_COUNT,
};

enum {
    DEFAULT_MIRROR_BACK_BUFFER_INDEX,
    DEFAULT_MIRROR_FRONT_BUFFER_INDEX,
    MIRROR_BUFFER_COUNT,
};

enum {
    NOTE_A,
    NOTE_A_SHARP,
    NOTE_B_FLAT = NOTE_A_SHARP,
    NOTE_B,
    NOTE_C,
    NOTE_C_SHARP,
    NOTE_D_FLAT = NOTE_C_SHARP,
    NOTE_D,
    NOTE_D_SHARP,
    NOTE_E_FLAT = NOTE_D_SHARP,
    NOTE_E,
    NOTE_F,
    NOTE_F_SHARP,
    NOTE_G_FLAT = NOTE_F_SHARP,
    NOTE_G,
    NOTE_G_SHARP,
    NOTE_A_FLAT = NOTE_G_SHARP,
    NOTE_COUNT,
    SILENCE,
};

enum {
    MEASURE_MELODY,
    MEASURE_HARMONY,
    MEASURE_TOTAL,
};

// structs:

typedef struct Tone {
    uint8_t note;
    uint8_t octave;

    // audio thread data
    float frequency;
    float sineIndex;
} Tone;

typedef struct Chord {
    uint8_t root;
    uint8_t third;
    uint8_t fifth;
} Chord;

// this is used for both singular tones and multi-tone chords
typedef struct MusicalEvent {
    uint8_t duration;
    Tone tones[MUSICAL_EVENT_MAX_TONES];
    int toneCount;
} MusicalEvent;

typedef struct Measure {
    int flags;
    MusicalEvent events[MEASURE_EVENT_CAPACITY];
    int eventCount;
    Chord chord;
} Measure;

typedef struct MeasurePlaybackState {
    _Atomic(int) eventIndex;
    _Atomic(float) cursorXPosition;

    // use only on audio thread
    unsigned int eventStartSample;
    unsigned int eventEndSample;
} MeasurePlaybackState;

typedef struct MusicBuffer {
    Measure measures[MEASURE_TOTAL];
} MusicBuffer;

typedef struct State {
    // this must be greater than zero for the randomness to work properly
    // TODO: is this guaranteed now?
    uint64_t randomState;

    // containing mirrorFrontBuffer, mirrorBackBuffer
    MusicBuffer mirrorBuffers[MIRROR_BUFFER_COUNT];

    int mirrorBackBufferIndex;
    int mirrorFrontBufferIndex;
} State;

typedef struct SharedState {
    _Atomic(bool) isAudioBackBufferPrepared;

    // main thread should only modify if .atomic.isAudioBackBufferPrepared is false
    // audio thread should only modify if .atomic.isAudioBackBufferPrepared is true
    int audioBackBufferIndex;

    // containing audioFrontBuffer, audioBackBuffer
    MusicBuffer audioBuffers[AUDIO_BUFFER_COUNT];

    MeasurePlaybackState measurePlaybackStates[MEASURE_TOTAL];
} SharedState;

typedef struct AudioThreadState {
    int audioFrontBufferIndex;
    unsigned int currentSample;
    int32_t bigBuffer[AUDIO_STREAM_SIZE];
} AudioThreadState;

// TODO: the non shared state pointer can be declared in main
static State *state = NULL;
static SharedState *sharedState = NULL;
static AudioThreadState *audioThreadState = NULL;

// common functions

static uint64_t NextRandom() {
    uint64_t x = state->randomState;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state->randomState = x;
    return x;
}

// TODO: debug only
static void dbgrec(int index, Color color) {
    int padding = 5;
    int side = 20;
    DrawRectangle((side + padding) * index, padding, side, side, color);
}

// TODO: debug only
static void dbgtext(int index, const char *text, Color color) {
    DrawText(text, 5, 5 + (index * 40), 20, color);
}

