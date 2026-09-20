static float GetGuiContainerPaddingSize() {
    return GetSmallestWindowDimension() / 300;
}

static float GetGuiContainerHeaderSize() {
    return GetSmallestWindowDimension() / 50;
}

static Rectangle GetGuiContainerExpandButtonRectangle(const GuiContainer *container) {
    return (Rectangle) {
        .x = container->headerRectangle.x,
        .y = container->headerRectangle.y,
        .width = container->headerRectangle.height, // yes height for square button
        .height = container->headerRectangle.height,
    };
}

static bool IsGuiContainerExpanded(const GuiContainer *container) {
    return hasAllFlags(container->flags, FLAG_GUI_CONTAINER_EXPANDED);
}

static void GuiContainerInitialize(GuiContainer containers[GUI_CONTAINER_COUNT]) {
    containers[GUI_CONTAINER_MELODY].name = "Melody Sequencer";
    containers[GUI_CONTAINER_MELODY].color = COLOR(.5f,.25f,.25f);
    containers[GUI_CONTAINER_MELODY].flags = FLAG_GUI_CONTAINER_EXPANDED;
    containers[GUI_CONTAINER_MELODY].buttonChar = '1';

    containers[GUI_CONTAINER_HARMONY].name = "Harmony Sequencer";
    containers[GUI_CONTAINER_HARMONY].color = COLOR(.25f,.5f,.25f);
    containers[GUI_CONTAINER_HARMONY].flags = FLAG_NONE;
    containers[GUI_CONTAINER_HARMONY].buttonChar = '2';

    containers[GUI_CONTAINER_MENU].name = "Menu";
    containers[GUI_CONTAINER_MENU].color = COLOR(.25f,.25f,.5f);
    containers[GUI_CONTAINER_MENU].flags = FLAG_NONE;
    containers[GUI_CONTAINER_MENU].buttonChar = '3';
}

static void GuiContainerUpdate(GuiContainer containers[GUI_CONTAINER_COUNT]) {
    float expandedContainers = 0;
    for (int i = 0; i < GUI_CONTAINER_COUNT; i++) {
        GuiContainer *container = &containers[i];

        int flags = container->flags;
        if (hasAllFlags(flags, FLAG_GUI_CONTAINER_EXPANDED_REQUEST)) {
            flags |= FLAG_GUI_CONTAINER_EXPANDED;
        } else {
            flags &= ~FLAG_GUI_CONTAINER_EXPANDED;
        }

        container->flags = flags;
        if (IsGuiContainerExpanded(container)) {
            expandedContainers++;
        }
    }

    const float paddingSize = GetGuiContainerPaddingSize();
    const float headerSize = GetGuiContainerHeaderSize();
    float expandedContainerHeight = 0;
    // (top padding) + (header) + (padding in between header and content) + (bottom padding)
    float collapsedContainerHeight = paddingSize + headerSize + paddingSize + paddingSize;
    float expandedContentHeight = 0;

    if (expandedContainers > 0) {
        expandedContainerHeight = (GetScreenHeight() - (collapsedContainerHeight * (GUI_CONTAINER_COUNT - expandedContainers))) / expandedContainers;
        expandedContentHeight = expandedContainerHeight - collapsedContainerHeight;
    }

    float globalHeight = 0;

    Vector2 mousePosition = GetMousePosition();
    bool mouseButtonPressed = IsMouseButtonPressed(0);

    for (int i = 0; i < GUI_CONTAINER_COUNT; i++) {
        GuiContainer *container = &containers[i];

        bool expanded = IsGuiContainerExpanded(container);

        container->outerRectangle = (Rectangle) {
            .x = 0,
            .y = globalHeight,
            .width = GetScreenWidth(),
            .height = expanded ? expandedContainerHeight : collapsedContainerHeight,
        };

        container->headerRectangle = (Rectangle) {
            .x = paddingSize,
            .y = container->outerRectangle.y + paddingSize,
            .width = container->outerRectangle.width - (2 * paddingSize),
            .height = headerSize,
        };

        container->contentRectangle = (Rectangle) {
            .x = paddingSize,
            .y = container->outerRectangle.y + paddingSize + container->headerRectangle.height + paddingSize,
            .width = container->headerRectangle.width,
            .height = expanded ? expandedContentHeight : 0,
        };

        Rectangle expandButtonRectangle = GetGuiContainerExpandButtonRectangle(container);

        bool keyboardActivation = IsKeyPressed(container->buttonChar);
        bool mouseActivation = mouseButtonPressed && CheckCollisionPointRec(mousePosition, expandButtonRectangle);

        if (keyboardActivation || mouseActivation) {
            // new dimensions etc happens next frame
            container->flags ^= FLAG_GUI_CONTAINER_EXPANDED_REQUEST;
        }

        globalHeight += container->outerRectangle.height;
    }
}

static void GuiContainerRender(const GuiContainer containers[GUI_CONTAINER_COUNT], Font font) {
    const float paddingSize = GetGuiContainerPaddingSize();
    const float headerSize = GetGuiContainerHeaderSize();
    const float fontSize = GetFontSize();

    for (int i = 0; i < GUI_CONTAINER_COUNT; i++) {
        const GuiContainer *container = &containers[i];

        DrawRectangleRec(container->outerRectangle, container->color);

        Rectangle expandButtonRectangle = GetGuiContainerExpandButtonRectangle(container);
        DrawRectangleRec(expandButtonRectangle, BLACK);

        {
            const char expandButtonText[] = { container->buttonChar, '\0' };
            const Vector2 expandButtonTextSize = MeasureTextEx(font, expandButtonText, fontSize, 0);
            const Vector2 expandButtonTextPosition = (Vector2){
                expandButtonRectangle.x + (expandButtonRectangle.width - expandButtonTextSize.x) / 2.0f,
                expandButtonRectangle.y + (expandButtonRectangle.height - expandButtonTextSize.y) / 2.0f,
            };
            const Vector2 expandButtonTextOrigin = {0};

            DrawTextPro(font, expandButtonText, expandButtonTextPosition, expandButtonTextOrigin, 0, fontSize, 0, WHITE);
        }

        Vector2 textPosition = {
            .x = expandButtonRectangle.x + expandButtonRectangle.width + paddingSize,
            .y = expandButtonRectangle.y + (headerSize / 2),
        };

        const Vector2 textSize = MeasureTextEx(font, container->name, fontSize, 0);
        const Vector2 origin = {
            0,
            textSize.y / 2,
        };

        DrawTextPro(font, container->name, textPosition, origin, 0.0f, fontSize, 0.0f, WHITE);
    }
}

