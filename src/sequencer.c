static float GetVisualMusicalEventWidth(const MusicalEvent *event, float measureWidth) {
    return ((float)event->duration / (float)DURATION_WHOLE) * measureWidth;
}

static float GetAbsoluteCursorXPosition(const Measure *measure, float measureWidth, Cursor cursor) {
    if (measure->eventCount == 0) {
        return 0;
    }

    ASSERT(cursor.eventIndex < measure->eventCount);

    float absoluteCursorXPosition = 0;

    int eventIndex;
    for (eventIndex = 0; eventIndex < cursor.eventIndex; eventIndex++) {
        float eventWidth = GetVisualMusicalEventWidth(&measure->events[eventIndex], measureWidth);
        absoluteCursorXPosition += eventWidth;
    }

    float currentEventWidth = GetVisualMusicalEventWidth(&measure->events[eventIndex], measureWidth);
    absoluteCursorXPosition += currentEventWidth * cursor.eventPosition;

    return absoluteCursorXPosition;
}

enum {
    MEASURE_TIME_DIMENSION_PAST = -1,
    MEASURE_TIME_DIMENSION_CURRENT,
    MEASURE_TIME_DIMENSION_FUTURE,
};

typedef struct MeasureRenderSettings {
    int measureTimeDimension;
    const MusicBuffer *buffer;
    int measureIndex;
    float localCursorXPosition;
    float absoluteCursorXPosition;
    int currentEventIndex;
    Rectangle measureBackground;
    float eventHeight;
} MeasureRenderSettings;

#define COLOR_MEASURE_BG COLOR(.1f,.1f,.1f)

#define COLOR_CHORD_NOTE_ACTIVE_FG COLOR(.5,1,.5)
#define COLOR_CHORD_NOTE_INACTIVE_FG COLOR(.25,.5,.25)

#define COLOR_SCALE_NOTE_ACTIVE_FG COLOR(.5,.5,1)
#define COLOR_SCALE_NOTE_INACTIVE_FG COLOR(.25,.25,.5)

#define COLOR_CHROMATIC_NOTE_ACTIVE_FG COLOR(1,.5,.5)
#define COLOR_CHROMATIC_NOTE_INACTIVE_FG COLOR(.5,.25,.25)

static void MeasureRender(MeasureRenderSettings settings) {
    const MusicBuffer *buffer = settings.buffer;
    const int measureIndex = settings.measureIndex;
    int measureTimeDimension = settings.measureTimeDimension;
    float localCursorXPosition = settings.localCursorXPosition;
    float absoluteCursorXPosition = settings.absoluteCursorXPosition;
    int currentEventIndex = settings.currentEventIndex;
    Rectangle measureBackground = settings.measureBackground;
    float eventHeight = settings.eventHeight;

    const Measure *measure = &buffer->measures[measureIndex];

    const float outdatedXBorder = measureBackground.x;
    const float upcomingXBorder = measureBackground.x + measureBackground.width;

    float eventOffset = 0.0f;
    for (int eventIndex = 0; eventIndex < measure->eventCount; eventIndex++) {
        const MusicalEvent *event = &measure->events[eventIndex];
        float eventWidth = GetVisualMusicalEventWidth(event, measureBackground.width);

        int cursorTimeDimension;
        switch (measureTimeDimension) {
            case MEASURE_TIME_DIMENSION_PAST:
            {
                cursorTimeDimension = CURSOR_AT_PAST_EVENT;
                break;
            }
            case MEASURE_TIME_DIMENSION_CURRENT:
            {
                if (eventIndex < currentEventIndex) {
                    cursorTimeDimension = CURSOR_AT_PAST_EVENT;
                } else if (eventIndex == currentEventIndex) {
                    cursorTimeDimension = CURSOR_AT_CURRENT_EVENT;
                } else {
                    cursorTimeDimension = CURSOR_AT_FUTURE_EVENT;
                }
                break;
            }
            case MEASURE_TIME_DIMENSION_FUTURE:
            {
                cursorTimeDimension = CURSOR_AT_FUTURE_EVENT;
                break;
            }
        }

        for (int toneIndex = 0; toneIndex < event->toneCount; toneIndex++) {
            Tone tone = event->tones[toneIndex];

            if (tone.note == SILENCE) {
                continue;
            }

            Rectangle rectangle;
            rectangle.x = measureBackground.x
                + measureBackground.width / 2
                + eventOffset
                - absoluteCursorXPosition;

            rectangle.width = eventWidth;

            // check outdated events:
            if (rectangle.x < outdatedXBorder) {
                if ((rectangle.x + rectangle.width) < outdatedXBorder) {
                    // fully offscreen
                    continue;
                }
                // partially offscreen
                rectangle.width = (rectangle.x + rectangle.width) - outdatedXBorder;
                rectangle.x = outdatedXBorder;
            }
            // check upcoming events
            else if ((rectangle.x + rectangle.width) > upcomingXBorder) {
                if (rectangle.x > upcomingXBorder) {
                    // fully offscreen
                    continue;
                }
                // partially offscreen
                rectangle.width = upcomingXBorder - rectangle.x;
            }

            rectangle.y = measureBackground.y
                + measureBackground.height
                - ((tone.note + 1) * eventHeight)
                + (LOWEST_OCTAVE * NOTES_PER_OCTAVE * eventHeight);

            rectangle.height = eventHeight;

            Color activeColor;
            Color inactiveColor;

            if (IsNoteInChord(buffer->chord, tone.note)) {
                activeColor = COLOR_CHORD_NOTE_ACTIVE_FG;
                inactiveColor = COLOR_CHORD_NOTE_INACTIVE_FG;
            } else if (IsNoteInScale(buffer->scale, tone.note)) {
                activeColor = COLOR_SCALE_NOTE_ACTIVE_FG;
                inactiveColor = COLOR_SCALE_NOTE_INACTIVE_FG;
            } else {
                activeColor = COLOR_CHROMATIC_NOTE_ACTIVE_FG;
                inactiveColor = COLOR_CHROMATIC_NOTE_INACTIVE_FG;
            }

            switch (cursorTimeDimension) {
                case CURSOR_AT_PAST_EVENT:
                    DrawRectangleRec(rectangle, inactiveColor);
                    break;
                case CURSOR_AT_CURRENT_EVENT:
                    DrawRectangleLinesEx(rectangle, 1, activeColor);
                    Rectangle progressRectangle = rectangle;
                    progressRectangle.width *= localCursorXPosition;
                    DrawRectangleRec(progressRectangle, activeColor);
                    break;
                case CURSOR_AT_FUTURE_EVENT:
                    DrawRectangleLinesEx(rectangle, 1, inactiveColor);
                    break;
            }
        }

        if (cursorTimeDimension == CURSOR_AT_CURRENT_EVENT) {
            Vector2 start = {
                measureBackground.x + (measureBackground.width / 2),
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

static void SequencerUpdate(const State *state, Sequencer *sequencer) {
    const MusicBuffer *visualFrontBuffer = GetReadonlyMirrorBuffer(state->mirrorBuffers, state->mirrorFrontBufferIndex);

    for (int measureIndex = 0; measureIndex < MEASURE_TOTAL; measureIndex++) {
        const MeasurePlaybackState *measurePlaybackState = &sharedState->measurePlaybackStates[measureIndex];

        int lastReportedEventIndex = atomic_load_explicit(&measurePlaybackState->eventIndex, memory_order_relaxed);
        float lastReportedEventPosition = atomic_load_explicit(&measurePlaybackState->eventPosition, memory_order_relaxed);
        double lastReportedTimestamp = atomic_load_explicit(&measurePlaybackState->timestamp, memory_order_relaxed);

        ASSERT(lastReportedEventIndex < MEASURE_EVENT_CAPACITY);

        const MusicalEvent *event = &visualFrontBuffer->measures[measureIndex].events[lastReportedEventIndex];
        float eventDuration = MusicalEventDuration(event);

        double sequencerTimestamp = GetTime();
        double elapsed = sequencerTimestamp - lastReportedTimestamp;
        double predictedEventPosition = lastReportedEventPosition + (elapsed / eventDuration);

        sequencer->cursors[measureIndex] = (Cursor) {
            .eventIndex = lastReportedEventIndex,
            .eventPosition = predictedEventPosition,
        };
    }
}

static void SequencerRender(const State *state) {
    // for drawing accidentals
    const Scale cMajorScale = CreateScaleFromType(NOTE_C, SCALE_MAJOR);

    // GUI_CONTAINER_MELODY is arbitrary here, they all have the same content width
    float measureWidth = state->guiContainers[GUI_CONTAINER_MELODY].contentRectangle.width;

    const MusicBuffer *visualFrontBuffer = GetReadonlyMirrorBuffer(state->mirrorBuffers, state->mirrorFrontBufferIndex);
    const MusicBuffer *visualBackBuffer = GetReadonlyMirrorBuffer(state->mirrorBuffers, state->mirrorBackBufferIndex);
    const MusicBuffer *visualOldBuffer = GetReadonlyMirrorBuffer(state->mirrorBuffers, DEFAULT_MIRROR_OLD_BUFFER_INDEX);

    for (int measureIndex = 0; measureIndex < MEASURE_TOTAL; measureIndex++) {
        const GuiContainer *guiContainer = NULL;
        switch (measureIndex) {
            default: ASSERT(false); break;
            case MEASURE_MELODY: guiContainer = &state->guiContainers[GUI_CONTAINER_MELODY]; break;
            case MEASURE_HARMONY: guiContainer = &state->guiContainers[GUI_CONTAINER_HARMONY]; break;
        }

        ASSERT(guiContainer != NULL);

        if (!IsGuiContainerExpanded(guiContainer)) {
            continue;
        }

        Rectangle measureBackground = guiContainer->contentRectangle;
        float eventHeight = measureBackground.height / (SEQUENCER_OCTAVE_COUNT * NOTES_PER_OCTAVE);

        DrawRectangleRec(measureBackground, COLOR_MEASURE_BG);
        DrawRectangleLinesEx(measureBackground, 2, BLACK);

        // TODO: this loop draws the background, but it currently looks kind of wacky, maybe remove or something
        for (int note = LOWEST_NOTE; note < HIGHEST_NOTE; note++) {
            int index = note - LOWEST_NOTE + 1;
            bool isAccidental = !IsNoteInScale(cMajorScale, note);
            Color color = isAccidental
                ? (Color){8,8,8,255}
                : (Color){16,16,16,255};

            Rectangle rectangle = {
                .x = measureBackground.x,
                .y = measureBackground.y + measureBackground.height - (eventHeight * index),
                .width = measureBackground.width,
                .height = eventHeight,
            };
            DrawRectangleRec(rectangle, color);
        }

        // relative to current event index (0 = start of currentEventIndex, 1.0 = end of currentEventIndex)
        const Measure *currentMeasure = &visualFrontBuffer->measures[measureIndex];

        Cursor cursor = state->sequencer.cursors[measureIndex];

        // relative to full visual measure in pixels
        const float absoluteCursorXPosition = GetAbsoluteCursorXPosition(currentMeasure, measureBackground.width, cursor);

        MeasureRenderSettings settings = {0};

        settings.localCursorXPosition = cursor.eventPosition;
        settings.currentEventIndex = cursor.eventIndex;
        settings.measureIndex = measureIndex;
        settings.measureBackground = measureBackground;
        settings.eventHeight = eventHeight;

        // old events from the previous front buffer that are still visible in sequencer
        settings.buffer = visualOldBuffer;
        settings.absoluteCursorXPosition = absoluteCursorXPosition + measureWidth;
        settings.measureTimeDimension = MEASURE_TIME_DIMENSION_PAST;
        MeasureRender(settings);

        // events in the current buffer
        settings.buffer = visualFrontBuffer;
        settings.absoluteCursorXPosition = absoluteCursorXPosition;
        settings.measureTimeDimension = MEASURE_TIME_DIMENSION_CURRENT;
        MeasureRender(settings);

        // upcoming events part the back buffer
        settings.buffer = visualBackBuffer;
        settings.absoluteCursorXPosition = absoluteCursorXPosition - measureWidth;
        settings.measureTimeDimension = MEASURE_TIME_DIMENSION_FUTURE;
        MeasureRender(settings);
    }
}

