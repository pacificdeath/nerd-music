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

#define DURATION_64TH (MEASURE_EVENT_CAPACITY / 64)
#define DURATION_32TH (MEASURE_EVENT_CAPACITY / 32)
#define DURATION_16TH (MEASURE_EVENT_CAPACITY / 16)
#define DURATION_8TH (MEASURE_EVENT_CAPACITY / 8)
#define DURATION_4TH (MEASURE_EVENT_CAPACITY / 4)
#define DURATION_HALF (MEASURE_EVENT_CAPACITY / 2)
#define DURATION_WHOLE (MEASURE_EVENT_CAPACITY)

#define INTERVAL_MAJOR_THIRD 4
#define INTERVAL_MINOR_THIRD 3
#define INTERVAL_PERFECT_FIFTH 7
#define INTERVAL_FLAT_FIFTH 6
#define INTERVAL_AUGMENTED_FIFTH 8
#define INTERVAL_MAJOR_SEVENTH 11
#define INTERVAL_MINOR_SEVENTH 10
#define INTERVAL_DIMINISHED_SEVENTH 9

typedef uint64_t flagtype;
#define FLAG_NONE ((uint64_t)0)
#define FLAG(x) ((uint64_t)1 << (x))

// chord triads:
#define FLAG_TRIAD_MAJOR                 (FLAG(0 ))
#define FLAG_TRIAD_MINOR                 (FLAG(1 ))
#define FLAG_TRIAD_DIMINISHED            (FLAG(2 ))
#define FLAG_TRIAD_AUGMENTED             (FLAG(3 ))

// 7th chords:
#define FLAG_CHORD_MAJOR_7               (FLAG(4 ) | FLAG_TRIAD_MAJOR)
#define FLAG_CHORD_MINOR_7               (FLAG(5 ) | FLAG_TRIAD_MINOR)
#define FLAG_CHORD_DOMINANT_7            (FLAG(6 ) | FLAG_TRIAD_MAJOR)
#define FLAG_CHORD_HALF_DIMINISHED_7     (FLAG(7 ) | FLAG_TRIAD_DIMINISHED)
#define FLAG_CHORD_MINOR_MAJOR_7         (FLAG(8 ) | FLAG_TRIAD_MINOR)
#define FLAG_CHORD_AUGMENTED_MAJOR_7     (FLAG(9 ) | FLAG_TRIAD_AUGMENTED)
#define FLAG_CHORD_FULLY_DIMINISHED_7    (FLAG(10) | FLAG_TRIAD_DIMINISHED)

#define SCALE_NOTE_CAPACITY 7
#define CHORD_NOTE_CAPACITY 4

#define NOTE_WITH_OCTAVE(note, octave) ((octave * NOTES_PER_OCTAVE) + note)
#define LOWEST_OCTAVE 2
#define HIGHEST_OCTAVE 6
#define LOWEST_NOTE NOTE_WITH_OCTAVE(0, LOWEST_OCTAVE)
#define HIGHEST_NOTE NOTE_WITH_OCTAVE((NOTES_PER_OCTAVE-1), HIGHEST_OCTAVE)
#define SEQUENCER_OCTAVE_COUNT (HIGHEST_OCTAVE - LOWEST_OCTAVE + 1)

#define MUSICAL_EVENT_MAX_TONES 4

#define MEASURE_EVENT_CAPACITY 64

#define MEASURE_FLAG_MUTED (FLAG(0))

#define VIEW_FLAG_MELODY (FLAG(0))
#define VIEW_FLAG_HARMONY (FLAG(1))
#define VIEW_FLAG_MENU (FLAG(2))

#define DEFAULT_VIEW_FLAGS (VIEW_FLAG_MELODY | VIEW_FLAG_HARMONY)

// colors
#define COLOR_BG (0.2f)
#define COLOR_INACTIVE (0.4f)
#define COLOR_ACTIVE (1.0f)
#define COLOR(r,g,b) ((Color){(r)*255.0f,(g)*255.0f,(b)*255.0f,255})
#define COLOR_MEASURE_BG COLOR(.1f,.1f,.1f)
// chord visuals
#define COLOR_CHORD_NOTE_BG COLOR(0,COLOR_BG,0)
#define COLOR_CHORD_NOTE_INACTIVE COLOR(0,COLOR_INACTIVE,0)
#define COLOR_CHORD_NOTE_ACTIVE COLOR(0,COLOR_ACTIVE,0)
// scale visuals
#define COLOR_SCALE_NOTE_BG COLOR(0,COLOR_BG/2,COLOR_BG)
#define COLOR_SCALE_NOTE_INACTIVE COLOR(0,COLOR_INACTIVE/2,COLOR_INACTIVE)
#define COLOR_SCALE_NOTE_ACTIVE COLOR(0,COLOR_ACTIVE/2,COLOR_ACTIVE)
// chromatic visuals
#define COLOR_CHROMATIC_NOTE_BG COLOR(COLOR_BG,0,0)
#define COLOR_CHROMATIC_NOTE_INACTIVE COLOR(COLOR_INACTIVE,0,0)
#define COLOR_CHROMATIC_NOTE_ACTIVE COLOR(COLOR_ACTIVE,0,0)

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
    NOTES_PER_OCTAVE,
    SILENCE,
};

enum {
    NOTE_TYPE_CHORD,
    NOTE_TYPE_SCALE,
    NOTE_TYPE_CHROMATIC,
};

enum {
    SCALE_MAJOR,
    SCALE_DORIAN,
    SCALE_PHRYGIAN,
    SCALE_LYDIAN,
    SCALE_MIXOLYDIAN,
    SCALE_MINOR,
    SCALE_LOCRIAN,
    SCALE_HARMONIC_MINOR,
    SCALE_MELODIC_MINOR,
    SCALE_COUNT,
};

enum {
    MEASURE_MELODY,
    MEASURE_HARMONY,
    MEASURE_TOTAL,
};

enum {
    CURSOR_AT_PAST_EVENT,
    CURSOR_AT_CURRENT_EVENT,
    CURSOR_AT_FUTURE_EVENT,
};

// structs:

typedef struct Tone {
    uint8_t note;

    // audio thread data
    float frequency;
    float sineIndex;
} Tone;

typedef struct Chord {
    uint8_t rootScaleDegree;
    union {
        struct {
            uint8_t root;
            uint8_t third;
            uint8_t fifth;
            uint8_t seventh;
        };
        uint8_t notes[CHORD_NOTE_CAPACITY];
    };
} Chord;

typedef struct Scale {
    uint8_t notes[SCALE_NOTE_CAPACITY];
} Scale;

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
    Scale scale;
    Chord chord;
} MusicBuffer;

#define MENU_HEIGHT 16
enum {
    MENU_STATE_OVERVIEW,
    MENU_STATE_SCALE,
    MENU_STATE_CHORD_PROGRESSION,
    MENU_STATE_COUNT,
};

typedef struct Menu {
    int state;
    int hightlightIndex;
    bool isHighlightInSubmenu;
    Rectangle outerRectangle;
    Rectangle innerRectangle;
} Menu;

typedef struct State {
    // this must be greater than zero for the randomness to work properly
    // TODO: is this guaranteed now?
    uint64_t randomState;

    // containing mirrorFrontBuffer, mirrorBackBuffer
    MusicBuffer mirrorBuffers[MIRROR_BUFFER_COUNT];

    int mirrorBackBufferIndex;
    int mirrorFrontBufferIndex;

    int viewFlags;
    float viewHeight;

    Menu menu;
} State;

typedef struct SharedState {
    _Atomic(bool) isAudioBackBufferPrepared;

    // main thread should only modify if .atomic.isAudioBackBufferPrepared is false
    // audio thread should only modify if .atomic.isAudioBackBufferPrepared is true
    int audioBackBufferIndex;

    // containing audioFrontBuffer, audioBackBuffer
    MusicBuffer audioBuffers[AUDIO_BUFFER_COUNT];

    MeasurePlaybackState measurePlaybackStates[MEASURE_TOTAL];

    // TODO: customizable at runtime, this has to be atomic basically
    float bpm;
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

static bool hasFlag(flagtype flags, flagtype flag) {
    return (flags & flag) == flag;
}

static uint64_t NextRandom() {
    uint64_t x = state->randomState;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state->randomState = x;
    return x;
}

static int NoOctave(int note) {
    return note % NOTES_PER_OCTAVE;
}

// TODO: debug only
// static void dbgrec(int index, Color color) {
//     int padding = 5;
//     int side = 20;
//     DrawRectangle((side + padding) * index, padding, side, side, color);
// }

// TODO: debug only
// static void dbgtext(int index, const char *text, Color color) {
//     DrawText(text, 5, 5 + (index * 40), 20, color);
// }

