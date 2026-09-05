static Scale CreateScale(
    uint8_t rootNote,
    uint8_t i,
    uint8_t ii,
    uint8_t iii,
    uint8_t iv,
    uint8_t v,
    uint8_t vi,
    uint8_t vii
) {
    Scale scale = {0};

    scale.notes[0] = NoOctave(rootNote + i);
    scale.notes[1] = NoOctave(rootNote + ii);
    scale.notes[2] = NoOctave(rootNote + iii);
    scale.notes[3] = NoOctave(rootNote + iv);
    scale.notes[4] = NoOctave(rootNote + v);
    scale.notes[5] = NoOctave(rootNote + vi);
    scale.notes[6] = NoOctave(rootNote + vii);

    return scale;
}

static Scale CreateScaleFromType(int rootNote, int scaleType) {
    switch (scaleType) {
        default: ASSERT(false); return (Scale){0};

        case SCALE_MAJOR:           return CreateScale(rootNote, 0, 2, 4, 5, 7, 9, 11);
        case SCALE_DORIAN:          return CreateScale(rootNote, 0, 2, 3, 5, 7, 9, 10);
        case SCALE_PHRYGIAN:        return CreateScale(rootNote, 0, 1, 3, 5, 7, 8, 10);
        case SCALE_LYDIAN:          return CreateScale(rootNote, 0, 2, 4, 6, 7, 9, 11);
        case SCALE_MIXOLYDIAN:      return CreateScale(rootNote, 0, 2, 4, 5, 7, 9, 10);
        case SCALE_MINOR:           return CreateScale(rootNote, 0, 2, 3, 5, 7, 8, 10);
        case SCALE_LOCRIAN:         return CreateScale(rootNote, 0, 1, 3, 5, 6, 8, 10);
        case SCALE_HARMONIC_MINOR:  return CreateScale(rootNote, 0, 2, 3, 5, 7, 8, 11);
        case SCALE_MELODIC_MINOR:   return CreateScale(rootNote, 0, 2, 3, 5, 7, 9, 11);
    }
}

static bool IsNoteInScale(Scale scale, int note) {
    note = NoOctave(note);
    for (int i = 0; i < SCALE_NOTE_CAPACITY; i++) {
        if (scale.notes[i] == note) {
            return true;
        }
    }
    return false;
}

// static Chord GetNextChordInProgression(Scale scale, Chord currentChord) {
//     Chord candidates[SCALE_NOTE_CAPACITY];
//     int nicenessLevels[SCALE_NOTE_CAPACITY];
//     int totalNiceness = 0;
//
//     for (int i = 0; i < SCALE_NOTE_CAPACITY; i++) {
//         candidates[i] = CreateChordFromScaleDegree(scale, i);
//
//         int niceness = GetChordProgressionNicenessLevel(currentChord, candidates[i]);
//         nicenessLevels[i] = niceness;
//         totalNiceness += niceness;
//     }
//
//     uint64_t random = NextRandom() % totalNiceness;
//
//     int chordIndex = 0;
//     int chordNiceness = nicenessLevels[0];
//     for (int i = 0; i < random; i++) {
//         if (i < chordNiceness) {
//             continue;
//         }
//
//         chordIndex++;
//         ASSERT(chordIndex < SCALE_NOTE_CAPACITY);
//         chordNiceness += nicenessLevels[chordIndex];
//     }
//
//     return candidates[chordIndex];
// }

