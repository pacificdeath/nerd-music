static Chord CreateChordFromScaleDegree(Scale scale, int degree) {
    ASSERT(degree >= 0);
    ASSERT(degree < SCALE_NOTE_CAPACITY);

    Chord chord = {0};
    chord.rootScaleDegree = degree;

    for (int i = 0; i < CHORD_NOTE_CAPACITY; i++) {
        int relativeDegree = (degree + (i * 2)) % SCALE_NOTE_CAPACITY;
        chord.notes[i] = scale.notes[relativeDegree];
    }

    return chord;
}

// static bool ChordEquals(Chord chord1, Chord chord2) {
//     for (int i = 0; i < CHORD_NOTE_CAPACITY; i++) {
//         if (chord1.notes[i] != chord2.notes[i]) {
//             return false;
//         }
//     }
//     return true;
// }

static bool IsNoteInChord(Chord chord, int note) {
    note = NoOctave(note);
    for (int i = 0; i < CHORD_NOTE_CAPACITY; i++) {
        if (chord.notes[i] == note) {
            return true;
        }
    }
    return false;
}

static int GetNoteIntervalFromRoot(uint8_t rootNote, uint8_t otherNote) {
    if (otherNote < rootNote) {
        otherNote += NOTES_PER_OCTAVE;
    }

    int diff = otherNote - rootNote;

    return diff;
}

static flagtype GetChordFlags(Chord chord) {
    flagtype flags = FLAG_NONE;

    int thirdInterval = GetNoteIntervalFromRoot(chord.root, chord.third);
    int fifthInterval = GetNoteIntervalFromRoot(chord.root, chord.fifth);
    int seventhInterval = GetNoteIntervalFromRoot(chord.root, chord.seventh);

    switch (thirdInterval) {
        default: ASSERT(false); return FLAG_NONE;
        case INTERVAL_MAJOR_THIRD:
            switch (fifthInterval) {
                default: ASSERT(false); return FLAG_NONE;
                case INTERVAL_PERFECT_FIFTH:
                    flags |= FLAG_TRIAD_MAJOR;
                    switch (seventhInterval) {
                        default: ASSERT(false); return FLAG_NONE;
                        case INTERVAL_MAJOR_SEVENTH: flags |= FLAG_CHORD_MAJOR_7; break;
                        case INTERVAL_MINOR_SEVENTH: flags |= FLAG_CHORD_DOMINANT_7; break;
                    }
                    break;
                case INTERVAL_AUGMENTED_FIFTH:
                    flags |= FLAG_TRIAD_AUGMENTED;
                    switch (seventhInterval) {
                        default: ASSERT(false); return FLAG_NONE;
                        case INTERVAL_MAJOR_SEVENTH: flags |= FLAG_CHORD_AUGMENTED_MAJOR_7; break;
                    }
                    break;
            }
            break;
        case INTERVAL_MINOR_THIRD:
            switch (fifthInterval) {
                default: ASSERT(false); return FLAG_NONE;
                case INTERVAL_PERFECT_FIFTH:
                    flags |= FLAG_TRIAD_MINOR;
                    switch (seventhInterval) {
                        default: ASSERT(false); return FLAG_NONE;
                        case INTERVAL_MINOR_SEVENTH: flags |= FLAG_CHORD_MINOR_7; break;
                        case INTERVAL_MAJOR_SEVENTH: flags |= FLAG_CHORD_MINOR_MAJOR_7; break;
                    }
                    break;
                case INTERVAL_FLAT_FIFTH:
                    flags |= FLAG_TRIAD_DIMINISHED;
                    switch (seventhInterval) {
                        default: ASSERT(false); return FLAG_NONE;
                        case INTERVAL_MINOR_SEVENTH: flags |= FLAG_CHORD_HALF_DIMINISHED_7; break;
                        case INTERVAL_DIMINISHED_SEVENTH: flags |= FLAG_CHORD_FULLY_DIMINISHED_7; break;
                    }
                    break;
            }
            break;
    }

    return flags;
}

// static int GetChordProgressionNicenessLevel(Chord fromChord, Chord toChord) {
//     int niceness = 0;
//
//     flagtype fromFlags = GetChordFlags(fromChord);
//     flagtype toFlags = GetChordFlags(toChord);
//
//     // SHARED NOTES
//     for (int i = 0; i < CHORD_NOTE_CAPACITY; i++) {
//         for (int j = 0; j < CHORD_NOTE_CAPACITY; j++) {
//             if (fromChord.notes[i] == toChord.notes[j]) {
//                 niceness++;
//                 break;
//             }
//         }
//     }
//
//     {
//         // DOMINANT -> TONIC
//         bool isDominantInterval = GetNoteIntervalFromRoot(fromChord.root, toChord.root) == INTERVAL_PERFECT_FIFTH;
//         bool toChordIsNotThatDissonant = hasFlag(toFlags, FLAG_TRIAD_MAJOR) || hasFlag(toFlags, FLAG_TRIAD_MINOR);
//         if (isDominantInterval && toChordIsNotThatDissonant) {
//             if (hasFlag(fromFlags, FLAG_CHORD_DOMINANT_7))
//                 niceness += 10;
//             else if (hasFlag(fromFlags, FLAG_TRIAD_MAJOR)) {
//                 // it is pretty much a "dominant to tonic" move but its missing the 7th
//                 niceness += 5;
//             }
//         }
//     }
//
//     return niceness;
// }

