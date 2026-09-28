#define CONTROLLER_BG COLOR(.1f,.1f,.1f)
#define CONTROLLER_BG2 COLOR(.2f,.2f,.2f)
#define CONTROLLER_BPM_SLIDER_COLOR COLOR(.0f,1.f,.0f)
#define CONTROLLER_PLAY_BUTTON_COLOR COLOR(1.f,.0f,.0f)

#define CONTROLLER_KEY_PLAY KEY_SPACE

// TODO: currently if you set bpm to 0 things will go weird, maybe it should be possible?
#define MIN_BPM 1
#define MAX_BPM 300

static float ControllerCircleRender(Vector2 center, float radius, float doof, Color color) {
    DrawCircleV(center, radius, CONTROLLER_BG2);

    float outerRadius = radius - doof;
    float innerRadius = outerRadius - doof;

    DrawRing(center, innerRadius, outerRadius, 0, 360, 32, color);

    return innerRadius;
}

static bool IsCircleButtonActivated(CircleButton button, int keyboardShortcut) {
    Vector2 mousePosition = GetMousePosition();

    bool mouseClick = IsMouseButtonPressed(0) && CheckCollisionPointCircle(mousePosition, button.center, button.radius);
    bool keyboardClick = IsKeyPressed(keyboardShortcut);

    return mouseClick || keyboardClick;
}

static void ControllerUpdate(const State *state, Controller *controller, Rectangle rectangle) {
    float bpm = state->bpm;

    controller->rectangle = rectangle;

    Rectangle bottomHalf = {
        .x = rectangle.x,
        .y = rectangle.y + (rectangle.height / 2),
        .width = rectangle.width,
        .height = rectangle.height / 2,
    };

    const float bigSidePadding = rectangle.width * 0.05f;
    const float padding = rectangle.width * 0.01f;
    const float sectionWidth = (rectangle.width - (bigSidePadding * 2)) / 3;

    Rectangle bpmRectangle;
    bpmRectangle.x = bigSidePadding + bottomHalf.x;
    bpmRectangle.y = bottomHalf.y + padding;
    bpmRectangle.width = sectionWidth;
    bpmRectangle.height = bottomHalf.height - (2 * padding);

    Vector2 bpmCircleCenter;
    {
        float offset = bpm - MIN_BPM;
        float range = MAX_BPM - MIN_BPM;
        bpmCircleCenter.x = bpmRectangle.x + ((offset / range) * bpmRectangle.width);
        bpmCircleCenter.y = bpmRectangle.y + (bpmRectangle.height / 2);
    }

    float bpmCircleRadius = bpmRectangle.height * 0.45f;

    Vector2 mousePosition = GetMousePosition();
    bool click = IsMouseButtonPressed(0);

    bool bpmHold = hasAllFlags(controller->flags, FLAG_CONTROLLER_SETTING_BPM);
    if (bpmHold) {
        if (!IsMouseButtonDown(0)) {
            controller->flags &= ~FLAG_CONTROLLER_SETTING_BPM;
            bpmHold = false;
        }
    } else if (click) {
        bool clickBpm = CheckCollisionPointCircle(mousePosition, bpmCircleCenter, bpmCircleRadius);
        if (clickBpm) {
            controller->flags |= FLAG_CONTROLLER_SETTING_BPM;
        }
    }

    if (bpmHold) {
        bpmCircleCenter.x = mousePosition.x;
        if (bpmCircleCenter.x < bpmRectangle.x) {
            bpmCircleCenter.x = bpmRectangle.x;
        } else if (bpmCircleCenter.x > (bpmRectangle.x + bpmRectangle.width)) {
            bpmCircleCenter.x = bpmRectangle.x + bpmRectangle.width;
        }

        float offset = bpmCircleCenter.x - bpmRectangle.x;
        float range = bpmRectangle.width;
        float newBpm = MIN_BPM + ((offset / range) * (MAX_BPM - MIN_BPM));

        atomic_store_explicit(&sharedState->bpm, newBpm, memory_order_relaxed);
    }

    controller->bpmRectangle = bpmRectangle;
    controller->bpmCircleCenter = bpmCircleCenter;
    controller->bpmCircleRadius = bpmCircleRadius;

    float bpmAnimationTimer = controller->bpmAnimationTimer;
    bpmAnimationTimer += GetFrameTime() * (bpm / 60.0f);
    while (bpmAnimationTimer > 1.0f) {
        bpmAnimationTimer -= bpmAnimationTimer;
    }

    controller->bpmAnimationTimer = bpmAnimationTimer;

    Rectangle playRectangle = {
        .x = bpmRectangle.x + bpmRectangle.width,
        .y = bpmRectangle.y,
        .width = sectionWidth,
        .height = bpmRectangle.height,
    };

    CircleButton playButton = {
        .center = {
            .x = playRectangle.x + (playRectangle.width / 2),
            .y = playRectangle.y + (playRectangle.height / 2),
        },
        .radius = bottomHalf.height * 0.45f,
    };

    CircleButton nextSongButton = {
        .center = {
            .x = playButton.center.x + (playButton.radius * 2),
            .y = playButton.center.y,
        },
        .radius = bottomHalf.height * 0.4f,
    };

    CircleButton previousSongButton = {
        .center = {
            .x = playButton.center.x - (playButton.radius * 2),
            .y = playButton.center.y,
        },
        .radius = bottomHalf.height * 0.4f,
    };

    controller->playButton = playButton;
    controller->nextSongButton = nextSongButton;
    controller->previousSongButton = previousSongButton;

    if (IsCircleButtonActivated(playButton, KEY_SPACE)) {
        if (hasAllFlags(controller->flags, FLAG_CONTROLLER_PLAY)) {
            controller->flags &= ~FLAG_CONTROLLER_PLAY;
        } else {
            controller->flags |= FLAG_CONTROLLER_PLAY;
        }
    }

    if (IsCircleButtonActivated(nextSongButton, KEY_SPACE)) {
        controller->flags |= FLAG_CONTROLLER_NEXT_SONG;
    }

    if (IsCircleButtonActivated(previousSongButton, KEY_SPACE)) {
        controller->flags |= FLAG_CONTROLLER_PREVIOUS_SONG;
    }
}

static void ControllerRender(const State *state, const Controller *controller, Font font) {
    float bpm = state->bpm;

    Rectangle rectangle = controller->rectangle;

    DrawRectangleRec(rectangle, CONTROLLER_BG);

    {
        Rectangle topHalf = {
            .x = rectangle.x,
            .y = rectangle.y,
            .width = rectangle.width,
            .height = rectangle.height / 2,
        };

        const char *songName = state->songName.chars;

        Vector2 textPosition = {
            .x = topHalf.x + (topHalf.width / 2),
            .y = topHalf.y + (topHalf.height / 2),
        };

        const int fontSize = GetFontSize();

        const Vector2 textSize = MeasureTextEx(font, songName, fontSize, 0);
        const Vector2 origin = {
            textSize.x / 2,
            textSize.y / 2,
        };

        DrawTextPro(font, songName, textPosition, origin, 0.0f, fontSize, 0.0f, GREEN);
    }

    Rectangle bpmRectangle = controller->bpmRectangle;
    float bpmCircleRadius = controller->bpmCircleRadius;
    Vector2 bpmCircleCenter = controller->bpmCircleCenter;
    float bpmAnimationTimer = controller->bpmAnimationTimer;

    float bpmAngularAnimation = bpmAnimationTimer * PI2;

    Rectangle bpmSliderRectangle;
    bpmSliderRectangle.x = bpmRectangle.x,
    bpmSliderRectangle.width = bpmRectangle.width,
    bpmSliderRectangle.height = bpmCircleRadius / 8,
    bpmSliderRectangle.y = bpmRectangle.y + ((bpmRectangle.height - bpmSliderRectangle.height) / 2),

    DrawRectangleRounded(bpmSliderRectangle, 1, 8, CONTROLLER_BPM_SLIDER_COLOR);

    float innerRadius = ControllerCircleRender(bpmCircleCenter, bpmCircleRadius, bpmSliderRectangle.height, CONTROLLER_BPM_SLIDER_COLOR);

    int bpmSatelliteAmount = 3;
    float bpmSatelliteSpacing = PI2 / bpmSatelliteAmount;

    float bpmSatelliteRadius = bpmSliderRectangle.height;
    float bpmSatelliteOrbitRadius = innerRadius - (bpmSatelliteRadius * 3.0f);

    for (int i = 0; i < bpmSatelliteAmount; i++) {
        float animation = (bpmSatelliteSpacing * i) + bpmAngularAnimation;
        Vector2 bpmSatelliteCenter = {
            bpmCircleCenter.x + (sinf(animation) * bpmSatelliteOrbitRadius),
            bpmCircleCenter.y + (-cosf(animation) * bpmSatelliteOrbitRadius),
        };

        DrawCircleV(bpmSatelliteCenter, bpmSatelliteRadius, CONTROLLER_BPM_SLIDER_COLOR);
    }

    {
        innerRadius = ControllerCircleRender(controller->playButton.center, controller->playButton.radius, bpmSliderRectangle.height, CONTROLLER_PLAY_BUTTON_COLOR);
        float playPulseSize = innerRadius - (bpmSliderRectangle.height * 2) + (sinf(bpmAngularAnimation) * bpmSliderRectangle.height);
        DrawCircleV(controller->playButton.center, playPulseSize, CONTROLLER_PLAY_BUTTON_COLOR);
    }

    {
        innerRadius = ControllerCircleRender(controller->nextSongButton.center, controller->nextSongButton.radius, bpmSliderRectangle.height, CONTROLLER_PLAY_BUTTON_COLOR);
        float nextSongPulseSize = innerRadius - (bpmSliderRectangle.height * 2) + (sinf(bpmAngularAnimation) * bpmSliderRectangle.height);
        DrawCircleV(controller->nextSongButton.center, nextSongPulseSize, CONTROLLER_PLAY_BUTTON_COLOR);
    }

    {
        innerRadius = ControllerCircleRender(controller->previousSongButton.center, controller->previousSongButton.radius, bpmSliderRectangle.height, CONTROLLER_PLAY_BUTTON_COLOR);
        float previousSongPulseSize = innerRadius - (bpmSliderRectangle.height * 2) + (sinf(bpmAngularAnimation) * bpmSliderRectangle.height);
        DrawCircleV(controller->previousSongButton.center, previousSongPulseSize, CONTROLLER_PLAY_BUTTON_COLOR);
    }
}

