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

#define MSG_WAIT_TIMES 5
#define STARTING_VELTE_STR 9

// draw the status bar
void drawStatusBar(Editor* editor) {
    pos(0, editor->width, &editor->a);
    processBG(&editor->a, DARK_GREY);
    processFG(&editor->a, WHITE);
    
    displayDraw(&editor->a, "Velte - ");

    // append the filaneme; if any, and other information
    for (int i = 0; i < editor->height; i++) {
        if (i == STARTING_VELTE_STR) {
            pos(i, editor->width, &editor->a);
            if (editor->filename != NULL) {
                writeToAppendBuffer(strlen(editor->filename), editor->filename, &editor->a);
                pos(i + strlen(editor->filename) + 1, editor->width, &editor->a);
            }
            else displayDraw(&editor->a, "Untitled ");
            displayDraw(&editor->a, "%d/%d ", 
                        editor->c.cursorX,
                        editor->c.utfJump);
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

// control keypresses
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

void* check_realloc(void* buff, size_t length) {
    void* tmp = realloc(buff, length);
    if (!tmp) {
        errorHandle("Velte: realloc");
    }
    return tmp;
}

void writeToAppendBuffer(size_t fLen, char* formattedString, App* a) {
    a->string = check_realloc(a->string, a->length + fLen);
    memcpy(&a->string[a->length], formattedString, fLen);
    a->length += fLen;
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

void writeDisplay(App* a) {
    write(STDOUT_FILENO, a->string, a->length);
    free(a->string);
    a->length = 0;
}

// prompts the user to something
char *editorPrompt(Editor* editor, char* msg) {
    uint32_t* strmsg = NULL;
    char* charMsg = NULL;
    size_t len = 0;
    size_t resultingLen;
    editor->pc.maskX = 0;
    editor->c.utfJump = 0;
    editor->pc.scrollMaskX = 0;

    strmsg = check_malloc((len + 1) * sizeof(uint32_t));
    strmsg[len] = '\0';

    while (1) {
        editor->m.isStatus = 0;
        clearDisplay(&editor->a);
        getWindowSize(editor);
        displayDraw(&editor->a, "\033[?7l");

        size_t spliceLen = len - editor->pc.scrollMaskX;
        charMsg = utftomb(&strmsg[editor->pc.scrollMaskX], spliceLen, &resultingLen);

        pos(0, editor->width - 2, &editor->a);
        displayDraw(&editor->a, "Ctrl+X: Cancel - %s", msg);
        pos(0, editor->width - 1, &editor->a);
        writeToAppendBuffer(resultingLen, charMsg, &editor->a);

        bufferDisplay(editor);
        editor->m.isStatus = 1;

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
                if (editor->pc.maskX > 0) {
                    size_t prev = editor->pc.maskX - 1;
                    memmove(&strmsg[prev], &strmsg[prev + 1], (len - prev - 1) * sizeof(uint32_t));
                    editor->pc.maskX--;
                    len--;
                }
                break;
            }
            case ARROW_LEFT:
                if (editor->pc.maskX > 0) editor->pc.maskX--;
                break;
            case ARROW_RIGHT:
                if (editor->pc.maskX < len) editor->pc.maskX++;
                break;
            default:
                if (c != '\0') {
                    strmsg = check_realloc(strmsg, (len + 2) * sizeof(uint32_t));
                    memmove(&strmsg[editor->pc.maskX + 1], &strmsg[editor->pc.maskX], (len - editor->pc.maskX) * sizeof(uint32_t));
                    strmsg[editor->pc.maskX] = c;
                    editor->pc.maskX++;
                    len++;

                    strmsg[len] = '\0';
                }
                break;

        }

        // do the same thing we did with the regular chorizontal scrolling
        editor->c.utfJump = 0;
        for (size_t i = editor->pc.scrollMaskX; i < editor->pc.maskX; i++) {
            // make the mask cursor line up with multi-width characters
            calculateCharacterWidth(editor, strmsg[i]);
        }
        if (editor->pc.maskX + editor->c.utfJump > (size_t)editor->height + editor->pc.scrollMaskX - 5) {
            editor->pc.scrollMaskX = editor->pc.maskX + editor->c.utfJump - editor->height + 5;
        }
        else if (editor->pc.maskX + editor->c.utfJump < (size_t)editor->pc.scrollMaskX && editor->pc.scrollMaskX > 0) {
            editor->pc.scrollMaskX = editor->pc.maskX + editor->c.utfJump;
        }
        editor->c.utfJump = 0;
        for (size_t i = editor->pc.scrollMaskX; i < editor->pc.maskX; i++) {
            calculateCharacterWidth(editor, strmsg[i]);
        }
    }
    free(strmsg);
    free(charMsg);
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
        processFG(&editor->a, WHITE);
        pos(0, editor->width - 2, &editor->a);
        displayDraw(&editor->a, editor->m.msg);
    }
    else {
        memset(editor->m.msg, 0, strlen(editor->m.msg));
    }
}

// show linenumbers on the screen
void lineNumShow(Editor* editor) {
    // disable default line wrapping
    displayDraw(&editor->a, "\033[?7l");
    char Num[32];
    int showLine = 1 + editor->c.scrollY;
    Row* row = &editor->row[showLine - 1];
    int yPos = 1;
    
    while (yPos < editor->width - 1) {
        int showStrLen = snprintf(Num, sizeof(Num), "%d", showLine);
        /*
            we want the background to be dark grey
            additionally, we also want the foreground to be a 
            lighter grey, unless if the cursor is on the line
        */
        if (editor->c.cursorY == showLine)
            processFG(&editor->a, WHITE);
        else processFG(&editor->a, LIGHTER_GREY);

        if (showLine <= editor->linenum) {
            // draw the line number to the screen
            pos((editor->config.disLine - (showStrLen) - 1), yPos, &editor->a);
            displayDraw(&editor->a, "%s ", Num);
            processFG(&editor->a, WHITE);

            // append the text in
            size_t spliceLen = row->tabs.tlen - editor->c.scrollX;
            if (spliceLen >= (size_t)editor->height - 6) spliceLen = editor->height - 6;
            char* toStr = utftomb(&row->tabs.tab[editor->c.scrollX], spliceLen, &spliceLen);
            writeToAppendBuffer(spliceLen, toStr, &editor->a);
            free(toStr);
        }
        showLine++;
        row++;
        yPos++;
    }
    reset(&editor->a);
}

// buffer 
void bufferDisplay(Editor* editor) {
    lineNumShow(editor);
    if (editor->m.isStatus)
        drawStatusBar(editor);
    systemShowMessage(editor);
    positionCursor(editor, &editor->a);
    writeDisplay(&editor->a);

    editor->a.string = NULL;
    editor->a.length = 0;
}

void initConfig(Editor* editor) {
    editor->config.tabcount = 4;
    editor->config.disLine = 6;
}

// init
void showDisplay(Editor* editor) {
    editor->c.cursorX = 0;
    editor->c.cursorY = 1;
    editor->c.tabX = 0;
    editor->modified = 0;
    editor->tabRem = 0;
    editor->c.scrollX = 0;
    editor->c.scrollY = 0;
    editor->a.string = NULL;
    editor->a.length = 0;
    editor->m.isStatus = 1;
    editor->c.utfJump = 0;
    editor->pc.maskX = 0;
    editor->pc.scrollMaskX = 0;
    seteditorMsg(editor, "", 0);

    while (1) {
        clearDisplay(&editor->a);
        getWindowSize(editor);
        bufferDisplay(editor);
        controlKeypresses(editor);
        scroll(editor);
    }
}
