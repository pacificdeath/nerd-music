#ifndef DEBUG
#error you are a horrible person
#endif

static void RunTests() {
    Scale scale = {0};
    Chord chord = {0};

    {
        // test all chords in the major scale

        scale = CreateScaleFromType(NOTE_C, SCALE_MAJOR);

        chord = CreateChordFromScaleDegree(scale, 0);
        ASSERT(chord.root == NOTE_C);
        ASSERT(GetChordFlags(chord) == (FLAG_TRIAD_MAJOR | FLAG_CHORD_MAJOR_7));

        chord = CreateChordFromScaleDegree(scale, 1);
        ASSERT(chord.root == NOTE_D);
        ASSERT(GetChordFlags(chord) == (FLAG_TRIAD_MINOR | FLAG_CHORD_MINOR_7));

        chord = CreateChordFromScaleDegree(scale, 2);
        ASSERT(chord.root == NOTE_E);
        ASSERT(GetChordFlags(chord) == (FLAG_TRIAD_MINOR | FLAG_CHORD_MINOR_7));

        chord = CreateChordFromScaleDegree(scale, 3);
        ASSERT(chord.root == NOTE_F);
        ASSERT(GetChordFlags(chord) == (FLAG_TRIAD_MAJOR | FLAG_CHORD_MAJOR_7));

        chord = CreateChordFromScaleDegree(scale, 4);
        ASSERT(chord.root == NOTE_G);
        ASSERT(GetChordFlags(chord) == (FLAG_TRIAD_MAJOR | FLAG_CHORD_DOMINANT_7));

        chord = CreateChordFromScaleDegree(scale, 5);
        ASSERT(chord.root == NOTE_A);
        ASSERT(GetChordFlags(chord) == (FLAG_TRIAD_MINOR | FLAG_CHORD_MINOR_7));

        chord = CreateChordFromScaleDegree(scale, 6);
        ASSERT(chord.root == NOTE_B);
        ASSERT(GetChordFlags(chord) == (FLAG_TRIAD_DIMINISHED | FLAG_CHORD_HALF_DIMINISHED_7));
    }

    {
        // harmonic minor is good for testing as it contains all current supported chords

        scale = CreateScaleFromType(NOTE_A, SCALE_HARMONIC_MINOR);

        chord = CreateChordFromScaleDegree(scale, 0);
        ASSERT(chord.root == NOTE_A);
        ASSERT(GetChordFlags(chord) == (FLAG_TRIAD_MINOR | FLAG_CHORD_MINOR_MAJOR_7));

        chord = CreateChordFromScaleDegree(scale, 1);
        ASSERT(chord.root == NOTE_B);
        ASSERT(GetChordFlags(chord) == (FLAG_TRIAD_DIMINISHED | FLAG_CHORD_HALF_DIMINISHED_7));

        chord = CreateChordFromScaleDegree(scale, 2);
        ASSERT(chord.root == NOTE_C);
        ASSERT(GetChordFlags(chord) == (FLAG_TRIAD_AUGMENTED | FLAG_CHORD_AUGMENTED_MAJOR_7));

        chord = CreateChordFromScaleDegree(scale, 3);
        ASSERT(chord.root == NOTE_D);
        ASSERT(GetChordFlags(chord) == (FLAG_TRIAD_MINOR | FLAG_CHORD_MINOR_7));

        chord = CreateChordFromScaleDegree(scale, 4);
        ASSERT(chord.root == NOTE_E);
        ASSERT(GetChordFlags(chord) == (FLAG_TRIAD_MAJOR | FLAG_CHORD_DOMINANT_7));

        chord = CreateChordFromScaleDegree(scale, 5);
        ASSERT(chord.root == NOTE_F);
        ASSERT(GetChordFlags(chord) == (FLAG_TRIAD_MAJOR | FLAG_CHORD_MAJOR_7));

        chord = CreateChordFromScaleDegree(scale, 6);
        ASSERT(chord.root == NOTE_G_SHARP);
        ASSERT(GetChordFlags(chord) == (FLAG_TRIAD_DIMINISHED | FLAG_CHORD_FULLY_DIMINISHED_7));
    }

    {
        // chord inversions

        scale = CreateScaleFromType(NOTE_C, SCALE_MAJOR);
        Chord chord = CreateChordFromScaleDegree(scale, 0);
        ChordInversion inversion = {0};
        int lowestNote = 0;
        int octave = 4;

        bool dontAllow7thBased = false;
        bool allow7thBased = true;

        // ROOT TRIAD
        lowestNote = NoteWithOctave(NOTE_C, octave);
        inversion = CreateLowChordInversion(chord, lowestNote, dontAllow7thBased);
        ASSERT(inversion.noteCount == CHORD_NOTE_CAPACITY_NO_SEVENTH);
        ASSERT(inversion.notes[0] == NoteWithOctave(NOTE_C, octave));
        ASSERT(inversion.notes[1] == NoteWithOctave(NOTE_E, octave));
        ASSERT(inversion.notes[2] == NoteWithOctave(NOTE_G, octave));

        // FIRST INVERSION TRIAD
        lowestNote = NoteWithOctave(NOTE_E, octave);
        inversion = CreateLowChordInversion(chord, lowestNote, dontAllow7thBased);
        ASSERT(inversion.noteCount == CHORD_NOTE_CAPACITY_NO_SEVENTH);
        ASSERT(inversion.notes[0] == NoteWithOctave(NOTE_E, octave));
        ASSERT(inversion.notes[1] == NoteWithOctave(NOTE_G, octave));
        ASSERT(inversion.notes[2] == NoteWithOctave(NOTE_C, octave + 1));

        // SECOND INVERSION TRIAD
        lowestNote = NoteWithOctave(NOTE_G, octave);
        inversion = CreateLowChordInversion(chord, lowestNote, dontAllow7thBased);
        ASSERT(inversion.noteCount == CHORD_NOTE_CAPACITY_NO_SEVENTH);
        ASSERT(inversion.notes[0] == NoteWithOctave(NOTE_G, octave));
        ASSERT(inversion.notes[1] == NoteWithOctave(NOTE_C, octave + 1));
        ASSERT(inversion.notes[2] == NoteWithOctave(NOTE_E, octave + 1));

        // ROOT 7TH CHORD
        lowestNote = NoteWithOctave(NOTE_C, octave);
        inversion = CreateLowChordInversion(chord, lowestNote, allow7thBased);
        ASSERT(inversion.noteCount == CHORD_NOTE_CAPACITY);
        ASSERT(inversion.notes[0] == NoteWithOctave(NOTE_C, octave));
        ASSERT(inversion.notes[1] == NoteWithOctave(NOTE_E, octave));
        ASSERT(inversion.notes[2] == NoteWithOctave(NOTE_G, octave));
        ASSERT(inversion.notes[3] == NoteWithOctave(NOTE_B, octave + 1));

        // FIRST INVERSION 7TH CHORD
        lowestNote = NoteWithOctave(NOTE_E, octave);
        inversion = CreateLowChordInversion(chord, lowestNote, allow7thBased);
        ASSERT(inversion.noteCount == CHORD_NOTE_CAPACITY);
        ASSERT(inversion.notes[0] == NoteWithOctave(NOTE_E, octave));
        ASSERT(inversion.notes[1] == NoteWithOctave(NOTE_G, octave));
        ASSERT(inversion.notes[2] == NoteWithOctave(NOTE_B, octave + 1));
        ASSERT(inversion.notes[3] == NoteWithOctave(NOTE_C, octave + 1));

        // SECOND INVERSION 7TH CHORD
        lowestNote = NoteWithOctave(NOTE_G, octave);
        inversion = CreateLowChordInversion(chord, lowestNote, allow7thBased);
        ASSERT(inversion.noteCount == CHORD_NOTE_CAPACITY);
        ASSERT(inversion.notes[0] == NoteWithOctave(NOTE_G, octave));
        ASSERT(inversion.notes[1] == NoteWithOctave(NOTE_B, octave + 1));
        ASSERT(inversion.notes[2] == NoteWithOctave(NOTE_C, octave + 1));
        ASSERT(inversion.notes[3] == NoteWithOctave(NOTE_E, octave + 1));

        // THIRD INVERSION 7TH CHORD
        lowestNote = NoteWithOctave(NOTE_B, octave);
        inversion = CreateLowChordInversion(chord, lowestNote, allow7thBased);
        ASSERT(inversion.noteCount == CHORD_NOTE_CAPACITY);
        ASSERT(inversion.notes[0] == NoteWithOctave(NOTE_B, octave));
        ASSERT(inversion.notes[1] == NoteWithOctave(NOTE_C, octave));
        ASSERT(inversion.notes[2] == NoteWithOctave(NOTE_E, octave));
        ASSERT(inversion.notes[3] == NoteWithOctave(NOTE_G, octave));

        // SHARP ROOT
        lowestNote = NoteWithOctave(NOTE_C_SHARP, octave);
        inversion = CreateLowChordInversion(chord, lowestNote, allow7thBased);
        ASSERT(inversion.noteCount == CHORD_NOTE_CAPACITY);
        ASSERT(inversion.notes[0] == NoteWithOctave(NOTE_E, octave));
        ASSERT(inversion.notes[1] == NoteWithOctave(NOTE_G, octave));
        ASSERT(inversion.notes[2] == NoteWithOctave(NOTE_B, octave + 1));
        ASSERT(inversion.notes[3] == NoteWithOctave(NOTE_C, octave + 1));

        // FLAT 7TH
        lowestNote = NoteWithOctave(NOTE_B_FLAT, octave);
        inversion = CreateLowChordInversion(chord, lowestNote, allow7thBased);
        ASSERT(inversion.noteCount == CHORD_NOTE_CAPACITY);
        ASSERT(inversion.notes[0] == NoteWithOctave(NOTE_B, octave));
        ASSERT(inversion.notes[1] == NoteWithOctave(NOTE_C, octave));
        ASSERT(inversion.notes[2] == NoteWithOctave(NOTE_E, octave));
        ASSERT(inversion.notes[3] == NoteWithOctave(NOTE_G, octave));
    }

    {
        // transitional chords

        Chord chord = CreateSecondaryDominantChord(NOTE_A);
        ASSERT(chord.root == NOTE_E);
        ASSERT(chord.third == NOTE_G_SHARP);
        ASSERT(chord.fifth == NOTE_B);
        ASSERT(chord.seventh == NOTE_D);

        chord = CreateSecondaryDominantChord(NOTE_A_FLAT);
        ASSERT(chord.root == NOTE_E_FLAT);
        ASSERT(chord.third == NOTE_G);
        ASSERT(chord.fifth == NOTE_B_FLAT);
        ASSERT(chord.seventh == NOTE_D_FLAT);

        chord = CreateDiminishedPassingChord(NOTE_A);
        ASSERT(chord.root == NOTE_A_FLAT);
        ASSERT(chord.third == NOTE_B);
        ASSERT(chord.fifth == NOTE_D);
        ASSERT(chord.seventh == NOTE_F);

        chord = CreateDiminishedPassingChord(NOTE_A_FLAT);
        ASSERT(chord.root == NOTE_G);
        ASSERT(chord.third == NOTE_B_FLAT);
        ASSERT(chord.fifth == NOTE_D_FLAT);
        ASSERT(chord.seventh == NOTE_E);
    }
}

