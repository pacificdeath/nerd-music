static Tone CreateTone(int note) {
#ifdef DEBUG
    if (note != SILENCE) {
        ASSERT(note > LOWEST_NOTE);
        ASSERT(note < HIGHEST_NOTE);
    }
#endif
    return (Tone) {
        .note = note,
    };
}

static Tone CreateToneWithOctave(int note, int octave) {
    ASSERT(note != SILENCE);
    ASSERT(note < NOTES_PER_OCTAVE);
    int absoluteNote = NOTE_WITH_OCTAVE(note, octave);
    return CreateTone(absoluteNote);
}

