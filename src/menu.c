#define MENU_BG_COLOR COLOR(.05f,.05f,.05f)
#define MENU_ITEM_BG_COLOR COLOR(.1f,.1f,.1f)

#define MENU_ITEM_BORDER_COLOR COLOR(.2f,.2f,.2f)
#define MENU_ITEM_BORDER_SELECTED_COLOR COLOR(.0f,.2f,.0f)

#define MENU_ITEM_FG_COLOR COLOR(1.f,1.f,1.f)
#define MENU_ITEM_FG_HOVER_COLOR COLOR(1.f,1.f,0.f)
#define MENU_ITEM_FG_SELECTED_COLOR COLOR(0.f,1.f,0.f)

enum {
    MENU_ITEM_ROOT_NOTE,
    MENU_ITEM_SCALE,
    MENU_ITEM_RHYTHM_TYPE,
};

static Rectangle FloatBoxToRectangle(const Menu *menu, FloatBox box) {
    return (Rectangle) {
        .x = menu->rectangle.x + (menu->rectangle.width * box.x),
        .y = menu->rectangle.y + (menu->rectangle.height * box.y),
        .width = menu->rectangle.width * box.width,
        .height = menu->rectangle.height * box.height,
    };
}

static void MenuInitialize(Menu *menu, const GuiContainer *guiContainer) {
    menu->guiContainer = guiContainer;
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
            .width = 0.5f / NOTES_PER_OCTAVE,
            .height = itemHeight,
        };

        for (int i = 0; i < NOTES_PER_OCTAVE; i++) {
            FloatBox box = base;
            box.x = base.x + (base.width * (i % NOTES_PER_OCTAVE));
            menu->items[itemIndex - NOTES_PER_OCTAVE + i].box = box;
        }
    }

    // initialize rhythm types
    {
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_RHYTHM_TYPE,
            .text = "Oompha",
            .value = RHYTHM_TYPE_OOMPHA,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_RHYTHM_TYPE,
            .text = "OomphaTriplet",
            .value = RHYTHM_TYPE_OOMPHA_TRIPLET,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_RHYTHM_TYPE,
            .text = "Waltz",
            .value = RHYTHM_TYPE_WALTZ,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_RHYTHM_TYPE,
            .text = "Arpeggio",
            .value = RHYTHM_TYPE_ARPEGGIO,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_RHYTHM_TYPE,
            .text = "FastArpeggio",
            .value = RHYTHM_TYPE_FAST_ARPEGGIO,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_RHYTHM_TYPE,
            .text = "ArpeggioTriplet",
            .value = RHYTHM_TYPE_ARPEGGIO_TRIPLET,
        };
        menu->items[itemIndex++] = (MenuItem) {
            .type = MENU_ITEM_RHYTHM_TYPE,
            .text = "FastArpeggioTriplet",
            .value = RHYTHM_TYPE_FAST_ARPEGGIO_TRIPLET,
        };

        FloatBox base = {
            .x = 0.5f,
            .y = itemHeight,
            .width = 0.5f / 2.0f,
            .height = itemHeight,
        };

        for (int i = 0; i < RHYTHM_TYPE_COUNT; i++) {
            FloatBox box = base;

            box.x = base.x + (base.width * (i % 2));
            box.y = base.y + (base.height * (i / 2));

            menu->items[itemIndex - RHYTHM_TYPE_COUNT + i].box = box;
        }
    }
}

static void MenuUpdate(Menu *menu) {
    ASSERT(menu->guiContainer != NULL);
    if (!IsGuiContainerExpanded(menu->guiContainer)) {
        return;
    }

    menu->rectangle = menu->guiContainer->contentRectangle;

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
            {
                int scaleType = item.value;
                ASSERT(scaleType >= 0);
                ASSERT(scaleType < SCALE_COUNT);
                menu->scale = CreateScaleFromType(menu->rootNote, scaleType);
                break;
            }
            case MENU_ITEM_ROOT_NOTE:
            {
                int rootNote = item.value;
                ASSERT(rootNote >= 0);
                ASSERT(rootNote < NOTES_PER_OCTAVE);
                menu->rootNote = rootNote;
                menu->scale = CreateScaleFromType(menu->rootNote, menu->scale.type);
                break;
            }
            case MENU_ITEM_RHYTHM_TYPE:
            {
                int rhythmType = item.value;
                ASSERT(rhythmType >= 0);
                ASSERT(rhythmType < RHYTHM_TYPE_COUNT);
                menu->rhythmType = rhythmType;
                break;
            }
        }
    }
}

static void MenuRender(const Menu *menu, Font font) {
    ASSERT(menu->guiContainer != NULL);
    if (!IsGuiContainerExpanded(menu->guiContainer)) {
        return;
    }

    DrawRectangleRec(menu->rectangle, MENU_BG_COLOR);

    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        MenuItem item = menu->items[i];
        Color fgColor = MENU_ITEM_FG_COLOR;
        Color borderColor = MENU_ITEM_BORDER_COLOR;
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
            case MENU_ITEM_RHYTHM_TYPE:
                if (menu->rhythmType == item.value) {
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

        const float fontSize = GetFontSize();
        const Vector2 textSize = MeasureTextEx(font, item.text, fontSize, 0);
        const Vector2 origin = {
            textSize.x / 2,
            textSize.y / 2,
        };

        DrawTextPro(font, item.text, textPosition, origin, 0.0f, fontSize, 0.0f, fgColor);
    }
}

