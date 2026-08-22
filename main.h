#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include "raylib.h"

// TODO: these debug things should not always be compiled
#define ASSERT(condition)\
    do { if (!(condition)) {\
        TraceLog(LOG_ERROR, "[you are a horrible person] %s:%i -> (%s)", __FILE__, __LINE__, #condition);\
        exit(1);\
    } } while (0)

// defines:

#define AUDIO_STREAM_SIZE 4096
#define SAMPLE_RATE 44100
#define SAMPLE_SIZE 16
#define CHANNELS 1

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

#define MUSICAL_EVENT_MAX_TONES 4

#define MEASURE_EVENT_CAPACITY 16

#define HAS_FLAG(flags,flag) ((flags&flag)==flag)
#define FLAG_NONE (0)

#define MEASURE_FLAG_MUTED (1 << 0)

// enums:

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
    float volume;
} Tone;

// this is used for both singular tones and multi-tone chords
typedef struct MusicalEvent {
    uint8_t duration;
    Tone tones[MUSICAL_EVENT_MAX_TONES];
    int toneCount;
} MusicalEvent;

// position between 0 -> 1 where 0 is start of tone and 1 is end of tone
typedef struct MeasurePosition {
    int eventIndex;
    float eventPosition;
} MeasurePosition;

typedef struct Measure {
    int flags;
    uint64_t startSample;
    MusicalEvent events[MEASURE_EVENT_CAPACITY];
    int eventCount;
    MeasurePosition position;
} Measure;

typedef struct State {
    Measure measures[MEASURE_TOTAL];
    int32_t bigBuffer[AUDIO_STREAM_SIZE];
    // this must be greater than zero for the randomness to work properly
    // TODO: is this guaranteed now?
    uint64_t randomState;
    uint64_t currentSample;
} State;

// hooray! global variables
static State *state;

// common functions

static uint64_t NextRandom() {
    uint64_t x = state->randomState;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state->randomState = x;
    return x;
}

