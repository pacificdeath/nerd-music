#define MENU_HEIGHT 16

enum {
    MENU_ITEM_ROOT_NOTE,
    MENU_ITEM_SCALE,
};

enum {
    INPUT_FIELD_BPM,
};

static float GetMenuLineHeight(const Menu *menu) {
    return menu->innerRectangle.height / MENU_HEIGHT;
}

static float GetMenuPadding(const Menu *menu) {
    return GetScreenWidth() / 200.0f;
}

static Rectangle FloatBoxToRectangle(const Menu *menu, FloatBox box) {
    return (Rectangle) {
        .x = menu->innerRectangle.x + (menu->innerRectangle.width * box.x),
        .y = menu->innerRectangle.y + (menu->innerRectangle.height * box.y),
        .width = menu->innerRectangle.width * box.width,
        .height = menu->innerRectangle.height * box.height,
    };
}

static void MenuInitialize(Menu *menu) {
    menu->font = LoadFontEx("ComicMono.ttf", 100, NULL, 0);
    menu->rootNote = NOTE_C;
    menu->scale = CreateScaleFromType(menu->rootNote, SCALE_MAJOR);

    int itemIndex = 0;
    float itemHeight = 1.0f / 10;

    // initialize scale buttons
    {
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_SCALE,
            .text = "Major",
            .value = SCALE_MAJOR,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_SCALE,
            .text = "Dorian",
            .value = SCALE_DORIAN,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_SCALE,
            .text = "Phrygian",
            .value = SCALE_PHRYGIAN,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_SCALE,
            .text = "Lydian",
            .value = SCALE_LYDIAN,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_SCALE,
            .text = "Mixolydian",
            .value = SCALE_MIXOLYDIAN,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_SCALE,
            .text = "Minor",
            .value = SCALE_MINOR,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_SCALE,
            .text = "locrian",
            .value = SCALE_LOCRIAN,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_SCALE,
            .text = "Harmonic Minor",
            .value = SCALE_HARMONIC_MINOR,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_SCALE,
            .text = "Melodic Minor",
            .value = SCALE_MELODIC_MINOR,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_SCALE,
            .text = "Double Harmonic",
            .value = SCALE_DOUBLE_HARMONIC,
        };

        FloatBox base = {
            .x = 0.0f,
            .y = 0.0f,
            .width = 0.5f,
            .height = itemHeight,
        };

        for (int i = 0; i < SCALE_COUNT; i++) {
            FloatBox box = base;
            box.y = base.y + (itemHeight * i);
            menu->items[itemIndex - SCALE_COUNT + i].box = box;
        }
    }

    // initialize root notes
    {
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_ROOT_NOTE,
            .text = "C",
            .value = NOTE_C,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_ROOT_NOTE,
            .text = "C#",
            .value = NOTE_C_SHARP,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_ROOT_NOTE,
            .text = "D",
            .value = NOTE_D,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_ROOT_NOTE,
            .text = "D#",
            .value = NOTE_D_SHARP,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_ROOT_NOTE,
            .text = "E",
            .value = NOTE_E,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_ROOT_NOTE,
            .text = "F",
            .value = NOTE_F,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_ROOT_NOTE,
            .text = "F#",
            .value = NOTE_F_SHARP,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_ROOT_NOTE,
            .text = "G",
            .value = NOTE_G,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_ROOT_NOTE,
            .text = "G#",
            .value = NOTE_G_SHARP,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_ROOT_NOTE,
            .text = "A",
            .value = NOTE_A,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_ROOT_NOTE,
            .text = "A#",
            .value = NOTE_A_SHARP,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_ROOT_NOTE,
            .text = "B",
            .value = NOTE_B,
        };

        FloatBox base = {
            .x = 0.5f,
            .y = 0.0f,
            .width = 0.5f / 3.0f,
            .height = itemHeight,
        };

        for (int i = 0; i < NOTES_PER_OCTAVE; i++) {
            FloatBox box = base;

            // rows of three
            box.x = base.x + (base.width * (i % 3));
            box.y = base.y + (base.height * (i / 3));

            menu->items[itemIndex - NOTES_PER_OCTAVE + i].box = box;
        }
    }
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

    Vector2 mousePosition = GetMousePosition();
    menu->hoverIndex = -1;

    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        MenuItem item = menu->items[i];
        Rectangle rectangle = FloatBoxToRectangle(menu, item.box);
        if (!CheckCollisionPointRec(mousePosition, rectangle)) {
            continue;
        }

        menu->hoverIndex = i;
        if (!IsMouseButtonPressed(0)) {
            continue;
        }

        switch (item.type) {
            default: ASSERT(false); break;
            case MENU_ITEM_SCALE:
                int scaleType = item.value;
                ASSERT(scaleType >= 0);
                ASSERT(scaleType < SCALE_COUNT);
                menu->scale = CreateScaleFromType(menu->rootNote, scaleType);
                break;
            case MENU_ITEM_ROOT_NOTE:
                int rootNote = item.value;
                ASSERT(rootNote >= 0);
                ASSERT(rootNote < NOTES_PER_OCTAVE);
                menu->rootNote = rootNote;
                break;
        }
    }
}

#define MENU_ITEM_BG_COLOR COLOR(.1f,.1f,.1f)

#define MENU_ITEM_BORDER_COLOR COLOR(.2f,.2f,.2f)
#define MENU_ITEM_BORDER_SELECTED_COLOR COLOR(.0f,.2f,.0f)

#define MENU_ITEM_FG_COLOR COLOR(1.f,1.f,1.f)
#define MENU_ITEM_FG_HOVER_COLOR COLOR(1.f,1.f,0.f)
#define MENU_ITEM_FG_SELECTED_COLOR COLOR(0.f,1.f,0.f)


static void MenuRender(const Menu *menu) {
    float lineHeight = GetMenuLineHeight(menu);
    float padding = GetMenuPadding(menu);
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        MenuItem item = menu->items[i];
        Color fgColor = MENU_ITEM_FG_COLOR;
        Color borderColor = MENU_ITEM_BORDER_COLOR;
        Color textColorSelected = GREEN;
        switch (item.type) {
            default:
                break;
            case MENU_ITEM_SCALE:
                if (menu->scale.type == item.value) {
                    fgColor = MENU_ITEM_FG_SELECTED_COLOR;
                    borderColor = MENU_ITEM_BORDER_SELECTED_COLOR;
                }
                break;
            case MENU_ITEM_ROOT_NOTE:
                if (menu->rootNote == item.value) {
                    fgColor = MENU_ITEM_FG_SELECTED_COLOR;
                    borderColor = MENU_ITEM_BORDER_SELECTED_COLOR;
                }
                // TODO:
                break;
        }

        if (menu->hoverIndex == i) {
            fgColor = MENU_ITEM_FG_HOVER_COLOR;
        }

        Rectangle rectangle = FloatBoxToRectangle(menu, item.box);
        DrawRectangleRec(rectangle, MENU_ITEM_BG_COLOR);
        DrawRectangleLinesEx(rectangle, 2, borderColor);

        Vector2 textPosition = {
            .x = rectangle.x + rectangle.width / 2,
            .y = rectangle.y + rectangle.height / 2,
        };

        const float fontSize = 30.0f;
        const Vector2 textSize = MeasureTextEx(menu->font, item.text, fontSize, 0);
        const Vector2 origin = {
            textSize.x / 2,
            textSize.y / 2,
        };

        DrawTextPro(menu->font, item.text, textPosition, origin, 0.0f, fontSize, 0.0f, fgColor);
    }
}

static void MenuDeinitialize(Menu *menu) {
    UnloadFont(menu->font);
}

