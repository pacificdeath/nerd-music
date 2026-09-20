void SequencerRender(const State *state) {
    const MusicBuffer *visualBuffer = GetReadonlyMirrorBuffer(state->mirrorBuffers, state->mirrorFrontBufferIndex);

    // for drawing accidentals
    const Scale cMajorScale = CreateScaleFromType(NOTE_C, SCALE_MAJOR);

    for (int measureIndex = 0; measureIndex < MEASURE_TOTAL; measureIndex++) {
        const GuiContainer *guiContainer = NULL;
        switch (measureIndex) {
            default: ASSERT(false); break;
            case MEASURE_MELODY: guiContainer = &state->guiContainers[GUI_CONTAINER_MELODY]; break;
            case MEASURE_HARMONY: guiContainer = &state->guiContainers[GUI_CONTAINER_HARMONY]; break;
        }

        if (!IsGuiContainerExpanded(guiContainer)) {
            continue;
        }

        ASSERT(guiContainer != NULL);

        const Measure *measure = &visualBuffer->measures[measureIndex];
        const MeasurePlaybackState *measurePlaybackState = &sharedState->measurePlaybackStates[measureIndex];

        const float cursorXPosition = atomic_load_explicit(&measurePlaybackState->cursorXPosition, memory_order_relaxed);
        const int currentEventIndex = atomic_load_explicit(&measurePlaybackState->eventIndex, memory_order_relaxed);

        Rectangle measureBackground = guiContainer->contentRectangle;

        Color backgroundColor;
        if (measureIndex == 0) {
            backgroundColor = COLOR_MEASURE_BG;
        } else {
            backgroundColor = COLOR_MEASURE_BG;
        }

        DrawRectangleRec(measureBackground, backgroundColor);
        DrawRectangleLinesEx(measureBackground, 2, BLACK);

        const float eventHeight = measureBackground.height / (SEQUENCER_OCTAVE_COUNT * NOTES_PER_OCTAVE);

        // TODO: this loop draws the background, but it currently looks kind of wacky, maybe remove or something
        for (int note = LOWEST_NOTE; note < HIGHEST_NOTE; note++) {
            int index = note - LOWEST_NOTE + 1;
            bool isAccidental = !IsNoteInScale(cMajorScale, note);
            Color color = isAccidental
                ? (Color){0,0,0,255}
                : (Color){16,16,16,255};

            Rectangle rectangle = {
                .x = measureBackground.x,
                .y = measureBackground.y + measureBackground.height - (eventHeight * index),
                .width = measureBackground.width,
                .height = eventHeight,
            };
            DrawRectangleRec(rectangle, color);
        }

        float eventOffset = 0.0f;
        for (int eventIndex = 0; eventIndex < measure->eventCount; eventIndex++) {
            const MusicalEvent *event = &measure->events[eventIndex];
            float eventWidth = ((float)event->duration / (float)DURATION_WHOLE) * measureBackground.width;

            int cursorTimeDimension;
            if (eventIndex < currentEventIndex) {
                cursorTimeDimension = CURSOR_AT_PAST_EVENT;
            } else if (eventIndex == currentEventIndex) {
                cursorTimeDimension = CURSOR_AT_CURRENT_EVENT;
            } else {
                cursorTimeDimension = CURSOR_AT_FUTURE_EVENT;
            }

            for (int toneIndex = 0; toneIndex < event->toneCount; toneIndex++) {
                Tone tone = event->tones[toneIndex];

                if (tone.note != SILENCE) {
                    Rectangle rectangle;
                    rectangle.x = measureBackground.x + eventOffset;

                    Color inactiveColor;
                    Color activeColor;
                    if (IsNoteInChord(visualBuffer->chord, tone.note)) {
                        activeColor = COLOR_CHORD_NOTE_ACTIVE;
                        inactiveColor = COLOR_CHORD_NOTE_INACTIVE;
                    } else if (IsNoteInScale(visualBuffer->scale, tone.note)) {
                        activeColor = COLOR_SCALE_NOTE_ACTIVE;
                        inactiveColor = COLOR_SCALE_NOTE_INACTIVE;
                    } else {
                        activeColor = COLOR_CHROMATIC_NOTE_ACTIVE;
                        inactiveColor = COLOR_CHROMATIC_NOTE_INACTIVE;
                    }

                    rectangle.y = measureBackground.y
                        + measureBackground.height
                        - ((tone.note + 1) * eventHeight)
                        + (LOWEST_OCTAVE * NOTES_PER_OCTAVE * eventHeight);

                    rectangle.width = eventWidth;
                    rectangle.height = eventHeight;

                    switch (cursorTimeDimension) {
                        case CURSOR_AT_PAST_EVENT:
                            DrawRectangleRec(rectangle, inactiveColor);
                            break;
                        case CURSOR_AT_CURRENT_EVENT:
                            DrawRectangleLinesEx(rectangle, 1, inactiveColor);
                            Rectangle progressRectangle = rectangle;
                            progressRectangle.width *= cursorXPosition;
                            DrawRectangleRec(progressRectangle, activeColor);
                            break;
                        case CURSOR_AT_FUTURE_EVENT:
                            DrawRectangleLinesEx(rectangle, 1, inactiveColor);
                            break;
                    }
                }
            }

            if (cursorTimeDimension == CURSOR_AT_CURRENT_EVENT) {
                Vector2 start = {
                    eventOffset + (cursorXPosition * eventWidth),
                    measureBackground.y,
                };
                Vector2 end = {
                    start.x,
                    measureBackground.y + measureBackground.height,
                };
                DrawLineEx(start, end, 2, YELLOW); // TODO
            }

            eventOffset += eventWidth;
        }
    }
}
