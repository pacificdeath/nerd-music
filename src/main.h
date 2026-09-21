#include <stdlib.h>
#include <math.h>
#include <stdatomic.h>

#include <stdint.h>

#include "raylib.h"

#ifdef DEBUG
#include <stdio.h>
#define ASSERT(condition)\
    do { if (!(condition)) {\
        printf("%s:%i: error: you are a horrible person (%s)\n", __FILE__, __LINE__, #condition);\
        exit(1);\
    } } while (0)
#else
#define ASSERT(condition)
#endif

// defines:

#define MIN(a, b) ((a)<(b)?(a):(b))

#define AUDIO_STREAM_SIZE 4096
#define SAMPLE_RATE 44100
#define SAMPLE_SIZE 16
#define CHANNELS 1

// least common multiple of 32 (smallest 4/4 beat) and 24 (smallest 3/4 beat)
#define MEASURE_EVENT_CAPACITY 96

// 4/4 measures
#define DURATION(divisor) (MEASURE_EVENT_CAPACITY / divisor)
#define DURATION_WHOLE DURATION(1)
#define DURATION_HALF DURATION(2)
#define DURATION_4 DURATION(4)
#define DURATION_8 DURATION(8)
#define DURATION_16 DURATION(16)
#define DURATION_32 DURATION(32)
// 3/4 measures
#define DURATION_6 DURATION(6)
#define DURATION_12 DURATION(12)
#define DURATION_24 DURATION(24)

#define INTERVAL_MAJOR_THIRD 4
#define INTERVAL_MINOR_THIRD 3
#define INTERVAL_PERFECT_FIFTH 7
#define INTERVAL_FLAT_FIFTH 6
#define INTERVAL_AUGMENTED_FIFTH 8
#define INTERVAL_MAJOR_SEVENTH 11
#define INTERVAL_MINOR_SEVENTH 10
#define INTERVAL_DIMINISHED_SEVENTH 9

#define MELODY_CHORD_NOTE_WEIGHT 5
#define MELODY_SCALE_NOTE_WEIGHT 4
#define MELODY_CHROMATIC_NOTE_WEIGHT 1

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

#define FLAG_GUI_CONTAINER_EXPANDED FLAG(0)
#define FLAG_GUI_CONTAINER_EXPANDED_REQUEST FLAG(1)

#define SCALE_NOTE_CAPACITY 7
#define CHORD_NOTE_CAPACITY 4
#define CHORD_NOTE_CAPACITY_NO_SEVENTH (CHORD_NOTE_CAPACITY - 1)

#define SECONDARY_DOMINANT_CHORD_FLAGS (FLAG_CHORD_MAJOR_7|FLAG_CHORD_MINOR_7)
#define DIMINISHED_PASSING_CHORD_FLAGS (FLAG_CHORD_MAJOR_7|FLAG_CHORD_MINOR_7)

#define NOTE_WITH_OCTAVE(note, octave) ((octave * NOTES_PER_OCTAVE) + note)
#define LOWEST_OCTAVE 2
#define HIGHEST_OCTAVE 7
#define LOWEST_NOTE NOTE_WITH_OCTAVE(0, LOWEST_OCTAVE)
#define HIGHEST_NOTE NOTE_WITH_OCTAVE((NOTES_PER_OCTAVE-1), HIGHEST_OCTAVE)
#define SEQUENCER_OCTAVE_COUNT (HIGHEST_OCTAVE - LOWEST_OCTAVE + 1)

#define MUSICAL_EVENT_MAX_TONES 4

#define MEASURE_FLAG_MUTED (FLAG(0))

#define COLOR(r,g,b) ((Color){(r)*255.0f,(g)*255.0f,(b)*255.0f,255})

enum {
    DEFAULT_AUDIO_BACK_BUFFER_INDEX = 0,
    DEFAULT_AUDIO_FRONT_BUFFER_INDEX,
    AUDIO_BUFFER_COUNT,

    DEFAULT_MIRROR_BACK_BUFFER_INDEX = 0,
    DEFAULT_MIRROR_FRONT_BUFFER_INDEX,
    DEFAULT_MIRROR_OLD_BUFFER_INDEX,
    MIRROR_BUFFER_COUNT,

    NOTE_A = 0,
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

    NOTE_TYPE_CHORD = 0,
    NOTE_TYPE_SCALE,
    NOTE_TYPE_CHROMATIC,
    NOTE_TYPES_COUNT,

    SCALE_MAJOR = 0,
    SCALE_DORIAN,
    SCALE_PHRYGIAN,
    SCALE_LYDIAN,
    SCALE_MIXOLYDIAN,
    SCALE_MINOR,
    SCALE_LOCRIAN,
    SCALE_HARMONIC_MINOR,
    SCALE_MELODIC_MINOR,
    SCALE_DOUBLE_HARMONIC,
    SCALE_COUNT,

    RHYTHM_TYPE_OOMPHA = 0,
    RHYTHM_TYPE_OOMPHA_TRIPLET,
    RHYTHM_TYPE_WALTZ,
    RHYTHM_TYPE_ARPEGGIO,
    RHYTHM_TYPE_FAST_ARPEGGIO,
    RHYTHM_TYPE_ARPEGGIO_TRIPLET,
    RHYTHM_TYPE_FAST_ARPEGGIO_TRIPLET,
    RHYTHM_TYPE_COUNT,

    GUI_CONTAINER_MELODY = 0,
    GUI_CONTAINER_HARMONY,
    GUI_CONTAINER_MENU,
    GUI_CONTAINER_COUNT,

    MEASURE_MELODY = 0,
    MEASURE_HARMONY,
    MEASURE_TOTAL,

    CURSOR_AT_PAST_EVENT = 0,
    CURSOR_AT_CURRENT_EVENT,
    CURSOR_AT_FUTURE_EVENT,
};

typedef struct Tone {
    int note;

    // audio thread data
    float frequency;
    float sineIndex;
} Tone;

typedef struct Chord {
    int rootScaleDegree;
    union {
        struct {
            int root;
            int third;
            int fifth;
            int seventh;
        };
        int notes[CHORD_NOTE_CAPACITY];
    };
} Chord;

typedef struct ChordInversion {
    int notes[CHORD_NOTE_CAPACITY];
    int noteCount; // depends on if it is a triad or 7th chord inversion
} ChordInversion;

#define CHORD_QUEUE_CAPACITY 4
typedef struct ChordQueue {
    Chord chords[CHORD_QUEUE_CAPACITY];
    int count;
} ChordQueue;

typedef struct Scale {
    int type;
    int notes[SCALE_NOTE_CAPACITY];
} Scale;

// this is used for both singular tones and multi-tone chords
typedef struct MusicalEvent {
    int duration;
    Tone tones[MUSICAL_EVENT_MAX_TONES];
    int toneCount;
} MusicalEvent;

typedef struct Measure {
    int flags;
    MusicalEvent events[MEASURE_EVENT_CAPACITY];
    int eventCount;
    Chord chord;
} Measure;

typedef struct Cursor {
    int eventIndex;
    float eventPosition;
} Cursor;

typedef struct MeasurePlaybackState {
    _Atomic(int) eventIndex;
    _Atomic(float) eventPosition;
    _Atomic(double) timestamp;

    // use only on audio thread
    unsigned int eventStartSample;
    unsigned int eventEndSample;
} MeasurePlaybackState;

typedef struct MusicBuffer {
    Measure measures[MEASURE_TOTAL];
    Scale scale;
    Chord chord;
    int rhythmType;
} MusicBuffer;

typedef struct FloatBox {
    float x;
    float y;
    float width;
    float height;
} FloatBox;

typedef struct MenuItem {
    int type;
    const char *text;
    int value;
    FloatBox box;
} MenuItem;

typedef struct GuiContainer {
    const char *name;
    Color color;
    flagtype flags;
    Rectangle outerRectangle;
    Rectangle headerRectangle;
    Rectangle contentRectangle;
    char buttonChar;
} GuiContainer;

typedef struct Sequencer {
    Rectangle rectangle;
    GuiContainer *guiContainers[MEASURE_TOTAL];
    Cursor cursors[MEASURE_TOTAL];
} Sequencer;

#define MENU_ITEM_COUNT (SCALE_COUNT + NOTES_PER_OCTAVE + RHYTHM_TYPE_COUNT)
typedef struct Menu {
    const GuiContainer *guiContainer;
    Rectangle rectangle;

    int rootNote;
    Scale scale;
    int rhythmType;

    MenuItem items[MENU_ITEM_COUNT];
    int hoverIndex;
} Menu;

typedef struct State {
    Font font;

    // containing mirrorFrontBuffer, mirrorBackBuffer, mirrorOldBuffer
    MusicBuffer mirrorBuffers[MIRROR_BUFFER_COUNT];

    int mirrorBackBufferIndex;
    int mirrorFrontBufferIndex;

    GuiContainer guiContainers[GUI_CONTAINER_COUNT];

    Sequencer sequencer;

    Menu menu;

    ChordQueue chordQueue;
} State;

typedef struct SharedState {
    _Atomic(bool) isAudioBackBufferPrepared;

    // main thread should only modify if .atomic.isAudioBackBufferPrepared is false
    // audio thread should only modify if .atomic.isAudioBackBufferPrepared is true
    int audioBackBufferIndex;

    int audioFrontBufferIndex;

    // containing audioFrontBuffer, audioBackBuffer
    MusicBuffer audioBuffers[AUDIO_BUFFER_COUNT];

    MeasurePlaybackState measurePlaybackStates[MEASURE_TOTAL];

    // TODO: customizable at runtime, this has to be atomic basically
    float bpm;

    // this must be greater than zero for the randomness to work properly
    // TODO: is this guaranteed now?
    uint64_t randomState;
} SharedState;

static SharedState *sharedState = NULL;

// common functions

static bool hasAllFlags(flagtype flags, flagtype flag) {
    return (flags & flag) == flag;
}

static bool hasAnyFlags(flagtype flags, flagtype flag) {
    return (flags & flag) != FLAG_NONE;
}

static int NoteWithOctave(int note, int octave) {
    return NOTE_WITH_OCTAVE(note, octave);
}

static int NoOctave(int note) {
    return note % NOTES_PER_OCTAVE;
}

static int GetOctave(int note) {
    return note / NOTES_PER_OCTAVE;
}

static int GetSmallestWindowDimension() {
    return (GetScreenWidth() < GetScreenHeight()) ? GetScreenWidth() : GetScreenHeight();
}

static float GetFontSize() {
    return GetSmallestWindowDimension() / 50;
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

