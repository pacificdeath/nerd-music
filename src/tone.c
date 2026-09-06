#define ASSERT_NOTE(note) do {\
    if (note != SILENCE) {\
        ASSERT(note > LOWEST_NOTE);\
        ASSERT(note < HIGHEST_NOTE);\
    }\
} while (0)

static Tone CreateTone(int note) {
    ASSERT(note);
    return (Tone) {
        .note = note,
    };
}

static Tone CreateToneWithOctave(int note, int octave) {
    ASSERT(note != SILENCE);
    ASSERT(note < NOTES_PER_OCTAVE);
    int absoluteNote = NoteWithOctave(note, octave);
    return CreateTone(absoluteNote);
}

