#include "display.h"
#include "error.h"
#include "keypresses.h"
#include "colour.h"
#include "uchar.h"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <limits.h>

#define MSG_WAIT_TIMES 5
#define STARTING_VELTE_STR 9
#define STARTING_SAVE_STR 18

// draw the status bar
void drawStatusBar(Editor* editor) {
    pos(0, editor->width, &editor->a);
    processBG(&editor->a, editor->config.colours[STATUS_COLOUR]);
    processFG(&editor->a, editor->config.colours[STATUS_TEXT_COLOUR]);
    
    displayDraw(&editor->a, "Velte - ");

    // append the filaneme; if any, and other information, such as if the file is modified or not
    for (int i = 0; i < editor->height; i++) {
        if (i == STARTING_VELTE_STR) {
            pos(i, editor->width, &editor->a);
            if (editor->filename != NULL) {
                displayDraw(&editor->a, "%s", editor->filename);
                pos(i + strlen(editor->filename) + 1, editor->width, &editor->a);
            }
            else displayDraw(&editor->a, "Untitled ");
            displayDraw(&editor->a, "%d/%d ", 
                        editor->c.cursorX,
                        editor->c.scrollY);
            if (editor->modified > 0) {
                displayDraw(&editor->a, "(modified)");
            }
        }
        else {
            displayDraw(&editor->a, " ");
        }
    }
    reset(&editor->a);
}

// get window size
void getWindowSize(Editor* editor) {
    struct winsize w;
    int f = open("/dev/tty", O_RDWR);

    if (f < 0 || ioctl(f, TIOCGWINSZ, &w) < 0) return;
    editor->width = w.ws_row + 1;
    editor->height = w.ws_col + 1;
}

// move the cursor to the required coordinates
void pos(size_t x, int y, App* a) {
    displayDraw(a, "\033[%d;%dH", (y), (x));
}

// get input from the user and process it
void controlKeypresses(Editor* editor) {
    uint32_t c = getMKeys();
    processKeypresses(c, editor);
}

// clear display
void clearDisplay(App* a) {
    displayDraw(a, "\033c");
}

// malloc and check if it failed so we don't have to do this every time
void* check_malloc(size_t length) {
    void* tmp = malloc(length);
    if (!tmp) {
        errorHandle("Velte: malloc");
    }
    return tmp;
}

// do the same for realloc
void* check_realloc(void* buff, size_t length) {
    void* tmp = realloc(buff, length);
    if (!tmp) {
        errorHandle("Velte: realloc");
    }
    return tmp;
}

// converts a hex string (such as 0F) to a size_t
int editorConvStringToSizeT(Editor* editor, size_t* conv, char* str, int base) {
    char* end;
    long toSizeT = strtol(str, &end, base);
    
    // making sure the value is in range
    if ((unsigned long)toSizeT > SIZE_MAX || toSizeT < 0) {
        seteditorMsg(editor, "Argument is out of range", 25);
        return 0;
    }
    if (*end != '\0') {
        seteditorMsg(editor, "Invalid number", 15);
        return 0;
    }
    *conv = toSizeT;
    return 1;
}

// check if a rgb value is valid and return the converted rgb value
// hexValue must always start with a '#'
int checkRgb(Editor* editor, Rgb* rgbVal, char* hexValue) {
    if (strlen(hexValue) != 7) return 0;
    if (hexValue[0] != '#') return 0;
    char hexCode[3];
    int j = 0;

    for (int i = 1; i < 7; i++) {
        hexCode[j] = hexValue[i];
        j++;
        if (j % 2 == 0) {
            hexCode[2] = '\0';
            size_t value;
            int result = editorConvStringToSizeT(editor, &value, hexCode, 16);
            if (!result || value > UCHAR_MAX) return 0;

            // check what part of the hex code corresponds to each colour, and insert
            // the values into the struct.
            if (i <= 2) rgbVal->r = (unsigned char)value;
            else if (i <= 4) rgbVal->g = (unsigned char)value;
            else rgbVal->b = (unsigned char)value;
            j = 0;
        }
    }
    return 1;
}

// write the string into the append buffer so that it can be displayed all at once to the screen
void writeToAppendBuffer(size_t fLen, char* formattedString, App* a) {
    a->string = check_realloc(a->string, a->length + fLen + 1);
    memcpy(&a->string[a->length], formattedString, fLen);
    a->length += fLen;
    a->string[a->length] = '\0';
}

// draw messages to the screen
int displayDraw(App* a, const char* format, ...) {
    va_list ap;
    va_start(ap, format);

    // get the length of the formatted string
    int length = vsnprintf(NULL, 0, format, ap);
    va_end(ap);

    va_start(ap, format);
    char* formattedString = check_malloc(length + 1);
    size_t fLen = vsprintf(formattedString, format, ap);

    writeToAppendBuffer(fLen, formattedString, a);

    va_end(ap);
    free(formattedString);
    return fLen;
}

// write the buffer out and free the buffer
void writeDisplay(App* a) {
    write(STDOUT_FILENO, a->string, a->length);
    free(a->string);
    
    a->string = NULL;
    a->length = 0;
}

// prompts the user to something
char *editorPrompt(Editor* editor, char* msg) {
    uint32_t* strmsg = NULL;
    char* charMsg = NULL;
    char* printMsg = NULL;
    size_t len = 0;
    size_t resultingLen;
    size_t printMsgLen;
    Cursor cursor;
    initCursor(&cursor);
    cursor.cursorY = editor->width - 2;

    strmsg = check_malloc((len + 1) * sizeof(uint32_t));
    strmsg[len] = '\0';

    while (1) {
        // format the message and set it as the editor message
        size_t spliceLen = len - cursor.scrollX;
        charMsg = utftomb(&strmsg[cursor.scrollX], spliceLen, &resultingLen);
        printMsgLen = strlen(msg) + resultingLen + STARTING_SAVE_STR;
        printMsg = check_malloc(printMsgLen + 1);

        int res = snprintf(printMsg, printMsgLen, "Ctrl-X Cancel - %s%s", msg, charMsg);
        if (res < 0) break;
        seteditorMsg(editor, printMsg, printMsgLen);
        
        // continue to let the display buffer
        bufferDisplay(editor);
        positionCursor(cursor, strlen(msg) + STARTING_SAVE_STR - 1, &editor->a);
        writeDisplay(&editor->a);
        seteditorMsg(editor, "", 0);

        // get a keypress from the user
        uint32_t c = getMKeys();
        switch (c) {
            case '\n':
            case '\r':
                // return the prompt entered
                return charMsg;
            case CTRL_KEY('x'):
                return 0;
            case BACKSPACE_KEY: {
                if (cursor.cursorX > 0) {
                    size_t prev = cursor.cursorX;
                    memmove(&strmsg[prev - 1], &strmsg[prev], (len - prev) * sizeof(uint32_t));
                    cursor.cursorX--;
                    len--;
                }
                break;
            }
            case ARROW_LEFT:
                if (cursor.cursorX > 0) cursor.cursorX--;
                break;
            case ARROW_RIGHT:
                if (cursor.cursorX < len) cursor.cursorX++;
                break;
            default:
                // TODO: put this into the append char function
                if (c != '\0') {
                    strmsg = check_realloc(strmsg, (len + 2) * sizeof(uint32_t));
                    memmove(&strmsg[cursor.cursorX + 1], &strmsg[cursor.cursorX], (len - cursor.cursorX) * sizeof(uint32_t));
                    strmsg[cursor.cursorX] = c;
                    cursor.cursorX++;
                    len++;

                    strmsg[len] = '\0';
                }
                break;

        }

        // do the same thing we did with the regular chorizontal scrolling
        cursor.utfJump = 0;
        for (size_t i = cursor.scrollX; i < cursor.cursorX; i++) {
            // make the mask cursor line up with multi-width characters
            calculateCharacterWidth(editor, strmsg[i], &cursor.utfJump);
        }
        // scroll the message if needed
        scroll(editor, &cursor, strmsg, strlen(msg) + STARTING_SAVE_STR - 1);
    }
    free(strmsg);
    free(charMsg);
    free(printMsg);
    return 0;
}

// set the display message to anything we want
void seteditorMsg(Editor* editor, char* msg, size_t length) {
    editor->m.msg = check_malloc(length + 1);
    memcpy(editor->m.msg, msg, length);
    editor->m.msg[length] = '\0';
    editor->m.time = time(NULL);
}

// display a message to the user
void systemShowMessage(Editor* editor) {
    if (time(NULL) < editor->m.time + MSG_WAIT_TIMES) {
        processFG(&editor->a, editor->config.colours[MESSAGE_COLOUR]);
        pos(0, editor->width - 2, &editor->a);
        displayDraw(&editor->a, editor->m.msg);
    }
    else {
        // clear it when the time has run out
        memset(editor->m.msg, 0, strlen(editor->m.msg));
    }
}

// show linenumbers on the screen
void lineNumShow(Editor* editor) {
    // disable default line wrapping
    displayDraw(&editor->a, "\033[?7l");
    int showLine = 1 + editor->c.scrollY;
    int yPos = 1;
    
    while (yPos < editor->width - 1) {
        int showStrLen = snprintf(0, 0, "%d", showLine);

        if (showLine <= editor->linenum) {
            Row* row = &editor->row[showLine - 1];
            if (editor->config.linenums) {
                // draw the line number to the screen
                // make sure the linenumber is coloured correctly
                if (editor->c.cursorY == (size_t)showLine)
                    processFG(&editor->a, editor->config.colours[LINENUM_TEXT_ACTIVE_COLOUR]);
                else 
                    processFG(&editor->a, editor->config.colours[LINENUM_TEXT_COLOUR]);
    
                pos(0, yPos, &editor->a);
                processBG(&editor->a, editor->config.colours[LINENUM_COLOUR]);
                for (size_t i = 0; i < editor->config.disLine - (showStrLen) - 2; i++) displayDraw(&editor->a, " ");
                displayDraw(&editor->a, "%d ", showLine);
            }
            else {
                pos(0, yPos, &editor->a);
            }

            // append the text in
            reset(&editor->a);
            size_t scrollX = 0;
            if (editor->c.cursorY == (size_t)showLine) scrollX = editor->c.scrollX;
            size_t spliceLen = row->tabs.tlen - scrollX;
            if (spliceLen >= (size_t)editor->height - 6) spliceLen = editor->height - 6;
            char* toStr = utftomb(&row->tabs.tab[scrollX], spliceLen, NULL);
            
            processBG(&editor->a, editor->config.colours[BACKGROUND_COLOUR]);
            displayDraw(&editor->a, "%s", toStr);
            free(toStr);
        }
        else {
            pos(0, yPos, &editor->a);
        }
        // coloring in the background
        processBG(&editor->a, editor->config.colours[BACKGROUND_COLOUR]);
        displayDraw(&editor->a, "\033[K");

        reset(&editor->a);
        showLine++;
        yPos++;
    }
}

// buffer 
void bufferDisplay(Editor* editor) {
    clearDisplay(&editor->a);
    lineNumShow(editor);
    getWindowSize(editor);   
    drawStatusBar(editor);
    systemShowMessage(editor);
}

void initConfig(Editor* editor) {
    editor->config.tabcount = DEFAULT_TABCOUNT;
    editor->config.disLine = DEFAULT_DISLINE;
    editor->config.linenums = true;

    // initialize velte colours, by default all colours will be transparent
    editor->config.colours = check_malloc(COLOUR_CONFIG_LENGTH * sizeof(Rgb));
    for (int i = 0; i < COLOUR_CONFIG_LENGTH; i++) {
        editor->config.colours[i].isTransparent = true;
    }

}

void initCursor(Cursor* cursor) {
    cursor->cursorX = 0;
    cursor->cursorY = 1;
    cursor->scrollX = 0;
    cursor->scrollY = 0;
    cursor->tabX = 0;
    cursor->utfJump = 0;
}
// init
void showDisplay(Editor* editor) {
    editor->modified = 0;
    editor->tabRem = 0;
    editor->a.string = NULL;
    editor->a.length = 0;
    seteditorMsg(editor, "", 0);
    initCursor(&editor->c);
    while (1) {
        bufferDisplay(editor);
        positionCursor(editor->c, editor->config.disLine, &editor->a);
        writeDisplay(&editor->a);
        controlKeypresses(editor);
        scroll(editor, &editor->c, editor->row[editor->c.cursorY-1].str, editor->config.disLine);
    }
}
