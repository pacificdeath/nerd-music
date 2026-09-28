#define TERMINAL_BG COLOR(.05f,.05f,.05f)
#define TERMINAL_FG COLOR(.8f,.8f,.8f)
#define TERMINAL_CURSOR_COLOR COLOR(1.f,.0f,.0f)

#define TERMINAL_VISIBLE_LINES 16

#define ASSERT_TERMINAL_LINE(line) ASSERT((line->count >=0) && (line->count <= TERMINAL_LINE_MAX_LENGTH))

enum {
    COMMAND_NONE,
    COMMAND_LIST,
    COMMAND_SET,
};

typedef struct CommandParseResult {
    int type;
} CommandParseResult;

static void TerminalLineCreate(TerminalLine *line) {
    line->count = 0;
}

static void TerminalLineAppendChar(TerminalLine *line, char c) {
    ASSERT_TERMINAL_LINE(line);
    if (line->count == TERMINAL_LINE_COUNT) {
        // can not add more chars
        return;
    }

    line->chars[line->count++] = c;
}

static void TerminalLineAppendString(TerminalLine *line, const char *string) {
    ASSERT_TERMINAL_LINE(line);
    if (line->count == TERMINAL_LINE_COUNT) {
        // can not add more chars
        return;
    }

    for (int i = 0; line->count < TERMINAL_LINE_MAX_LENGTH; i++) {
        char c = string[i];
        if (c == '\0') {
            break;
        }
        line->chars[line->count++] = c;
    }
}

static void TerminalLineMove(TerminalLine *destination, TerminalLine *source) {
    ASSERT_TERMINAL_LINE(destination);
    ASSERT_TERMINAL_LINE(source);

    for (int i = 0; i < source->count; i++) {
        destination->chars[i] = source->chars[i];
    }

    destination->count = source->count;
    source->count = 0;
}

static void TerminalLineRemoveChars(TerminalLine *line, int charAmount) {
    ASSERT_TERMINAL_LINE(line);
    int newCount = line->count - charAmount;
    line->count = (newCount < 0) ? 0 : newCount;
}

static void TerminalLineRender(const TerminalLine *line, Rectangle rectangle, int lineIndex, Font font) {
    ASSERT_TERMINAL_LINE(line);
    ASSERT(lineIndex < TERMINAL_VISIBLE_LINES);

    const float fontSize = GetFontSize();

    Vector2 position;
    position.y = rectangle.y
        + rectangle.height
        - ((rectangle.height / TERMINAL_VISIBLE_LINES) * lineIndex)
        - (fontSize * 1.5f);

    for (int i = 0; i < line->count; i++) {
        position.x = rectangle.x + ((rectangle.width / TERMINAL_LINE_MAX_LENGTH) * i);
        DrawTextCodepoint(font, line->chars[i], position, fontSize, TERMINAL_FG);
    }
}

static void TerminalInitialize(Terminal *terminal, const GuiContainer *guiContainer) {
    terminal->guiContainer = guiContainer;
    terminal->rootNote = NOTE_C;
    terminal->scale = CreateScaleFromType(terminal->rootNote, SCALE_MAJOR);
    terminal->lineCount = 1;
}

static void TerminalExtractWord(const TerminalLine *line, int offset, char outWord[TERMINAL_LINE_MAX_LENGTH + 1], int *outWordLength) {
    ASSERT(*outWordLength == 0);
    ASSERT(offset <= TERMINAL_LINE_MAX_LENGTH);

    int i;
    for (i = offset; i < line->count; i++) {
        if (i == TERMINAL_LINE_MAX_LENGTH) {
            break;
        }
        if (line->chars[i] == ' ') {
            break;
        }
        outWord[(*outWordLength)++] = line->chars[i];
    }

    outWord[i] = '\0';
}

static bool TerminalLineEq(const TerminalLine *line, const char *string) {
    int i;
    for (i = 0; i < line->count; i++) {
        char c = string[i];
        if (c == '\0') {
            break;
        }
        if (c != line->chars[i]) {
            return false;
        }
    }

    return i == line->count;
}

static CommandParseResult TerminalParseCommand(const TerminalLine *line) {
    ASSERT_TERMINAL_LINE(line);

    CommandParseResult result;

    char command[TERMINAL_LINE_MAX_LENGTH + 1];
    int commandLength = 0;
    TerminalExtractWord(line, 0, command, &commandLength);

    if (TextIsEqual(command, "set")) {
        result.type = COMMAND_SET;
    } else if (TextIsEqual(command, "list")) {
        result.type = COMMAND_LIST;
    } else {
        result.type = COMMAND_NONE;
    }

    char value[TERMINAL_LINE_MAX_LENGTH + 1];
    int valueLength = 0;
    TerminalExtractWord(line, 0, value, &valueLength);

    return result;
}

static void TerminalUpdate(Terminal *terminal) {
    if (!IsGuiContainerExpanded(terminal->guiContainer)) {
        return;
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
        return;
    }

    TerminalLine *inputLine = &terminal->inputLine;
    for (int key = KEY_A; key <= KEY_Z; key++) {
        if (IsKeyPressed(key)) {
            char lowercase = key + 32;
            TerminalLineAppendChar(inputLine, lowercase);
        }
    }
    for (int key = KEY_ONE; key <= KEY_NINE; key++) {
        if (IsKeyPressed(key)) {
            TerminalLineAppendChar(inputLine, key);
        }
    }
    if (IsKeyPressed(KEY_SPACE)) {
        TerminalLineAppendChar(inputLine, ' ');
    }
    if (IsKeyPressed(KEY_BACKSPACE)) {
        TerminalLineRemoveChars(inputLine, 1);
    }
    if (IsKeyPressed(KEY_ENTER)) {
        CommandParseResult parseResult = TerminalParseCommand(inputLine);
        switch (parseResult.type) {
            case COMMAND_NONE:
                TraceLog(LOG_ERROR, "NONE");
                break;
            case COMMAND_LIST:
                TraceLog(LOG_ERROR, "LIST");
                break;
            case COMMAND_SET:
                TraceLog(LOG_ERROR, "SET");
                break;
        }

        for (int i = (TERMINAL_LINE_COUNT - 1); i >= 1; i--) {
            TerminalLine *destination = &terminal->lines[i];
            TerminalLine *source = &terminal->lines[i - 1];
            TerminalLineMove(destination, source);
        }

        {
            // move input line to the start of the output terminal
            TerminalLine *destination = &terminal->lines[0];
            TerminalLineMove(destination, inputLine);
        }

        if (terminal->lineCount < TERMINAL_LINE_COUNT) {
            terminal->lineCount++;
        }
    }
}

static void TerminalRender(const Terminal *terminal, Font font) {
    if (!IsGuiContainerExpanded(terminal->guiContainer)) {
        return;
    }

    const TerminalLine *inputLine = &terminal->inputLine;

    Rectangle rectangle = terminal->guiContainer->contentRectangle;

    DrawRectangleRec(rectangle, TERMINAL_BG);

    Vector2 separatorStart = {
        .x = rectangle.x + (rectangle.width / 2),
        .y = rectangle.y,
    };

    Vector2 separatorEnd = {
        .x = separatorStart.x,
        .y = rectangle.y + (rectangle.height),
    };

    DrawLineEx(separatorStart, separatorEnd, 2, TERMINAL_FG);

    Rectangle inputRectangle = rectangle;
    inputRectangle.width /= 2;

    Rectangle outputRectangle = inputRectangle;
    outputRectangle.x += outputRectangle.width;

    TerminalLineRender(inputLine, inputRectangle, 0, font);

    for (int lineIndex = 0; lineIndex < TERMINAL_VISIBLE_LINES; lineIndex++) {
        const TerminalLine *line = &terminal->lines[lineIndex];
        TerminalLineRender(line, outputRectangle, lineIndex, font);
    }

    {
        float width = inputRectangle.width / TERMINAL_LINE_MAX_LENGTH;
        float height = inputRectangle.height / TERMINAL_VISIBLE_LINES;
        Rectangle cursorRectangle = {
            .x = inputRectangle.x + (inputLine->count * width),
            .y = inputRectangle.y + inputRectangle.height - height,
            .width = width,
            .height = height,
        };

        DrawRectangleRec(cursorRectangle, TERMINAL_CURSOR_COLOR);
    }
}

