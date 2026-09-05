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
}

