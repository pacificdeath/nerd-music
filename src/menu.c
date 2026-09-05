static float GetMenuLineHeight(const Menu *menu) {
    return menu->innerRectangle.height / MENU_HEIGHT;
}

static float GetMenuPadding(const Menu *menu) {
    return GetScreenWidth() / 200.0f;
}

static void MenuUpdate(Menu *menu, Rectangle outerRectangle) {
    menu->outerRectangle = outerRectangle;
    const float padding = GetMenuPadding(menu);
    menu->innerRectangle = (Rectangle){
        .x = outerRectangle.x + padding,
        .y = outerRectangle.y + padding,
        .width = outerRectangle.width - (padding * 2),
        .height = outerRectangle.height - (padding * 2),
    };

    float lineHeight = GetMenuLineHeight(menu);
    Vector2 mousePosition = GetMousePosition();
    menu->hightlightIndex = (mousePosition.y - menu->innerRectangle.y) / lineHeight;

    menu->isHighlightInSubmenu = mousePosition.x > (GetScreenWidth() / 2);

    if (IsMouseButtonPressed(0)) {
        if (!menu->isHighlightInSubmenu) {
            if (menu->hightlightIndex < MENU_STATE_COUNT) {
                menu->state = menu->hightlightIndex;
            }
        }
    }
}

static void ScaleMenuRender(const Menu *menu) {
    const float lineHeight = GetMenuLineHeight(menu);
    const char *text = NULL;
    for (int i = 0; i < SCALE_COUNT; i++) {
        switch (i) {
            default:
                ASSERT(false);
                break;
            case SCALE_MAJOR:
                text = "Major";
                break;
            case SCALE_DORIAN:
                text = "Dorian";
                break;
            case SCALE_PHRYGIAN:
                text = "Phrygian";
                break;
            case SCALE_LYDIAN:
                text = "Lydian";
                break;
            case SCALE_MIXOLYDIAN:
                text = "Mixolydian";
                break;
            case SCALE_MINOR:
                text = "Minor";
                break;
            case SCALE_LOCRIAN:
                text = "Locrian";
                break;
            case SCALE_HARMONIC_MINOR:
                text = "Harmonic Minor";
                break;
            case SCALE_MELODIC_MINOR:
                text = "Melodic Minor";
                break;
        }

        bool isHighlighed = menu->isHighlightInSubmenu && (menu->hightlightIndex == i);

        float y = menu->innerRectangle.y + (lineHeight * i);

        Color textColor = isHighlighed ? YELLOW : WHITE;
        DrawText(text, menu->innerRectangle.x + (GetScreenWidth() / 2), y, 20, textColor);
    }
}

static void MenuRender(const Menu *menu) {
    float lineHeight = GetMenuLineHeight(menu);
    float padding = GetMenuPadding(menu);
    const char *text = NULL;
    for (int i = 0; i < MENU_STATE_COUNT; i++) {
        switch (i) {
            default:
                ASSERT(false);
                break;
            case MENU_STATE_OVERVIEW:
                text = "Overview";
                break;
            case MENU_STATE_SCALE:
                text = "Scale";
                break;
            case MENU_STATE_CHORD_PROGRESSION:
                text = "Chord progression";
                break;
        }

        bool isHighlighed = !menu->isHighlightInSubmenu && (menu->hightlightIndex == i);
        bool isSelected = menu->state == i;

        float y = menu->innerRectangle.y + (lineHeight * i);

        if (isSelected) {
            Rectangle rectangle = {
                .x = menu->innerRectangle.x,
                .y = y,
                .width = (menu->innerRectangle.width / 2) - padding,
                .height = lineHeight,
            };
            DrawRectangleRec(rectangle, BLUE);
        }

        Color textColor = isHighlighed ? YELLOW : WHITE;
        DrawText(text, menu->innerRectangle.x, y, 20, textColor);
    }

    switch (menu->state) {
        case MENU_STATE_OVERVIEW:
            text = "Overview";
            break;
        case MENU_STATE_SCALE:
            ScaleMenuRender(menu);
            break;
        case MENU_STATE_CHORD_PROGRESSION:
            text = "Chord progression";
            break;
    }
}
