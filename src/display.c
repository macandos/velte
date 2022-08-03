#include "display.h"
#include "error.h"
#include "keypresses.h"
#include "colour.h"
#include "tabs.h"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>

#define MSG_WAIT_TIMES 5

// draw the status bar
void drawStatusBar(App* a, DisplayInit* dinit) {
    pos(0, dinit->height, a);
    processBG(a, DARK_GREY);
    processFG(a, WHITE);
    
    app(18, "Velte Text Editor, ", a);

    // append the filaneme; if any
    // if its null, then just say 'Untitled'
    char fn[64];
    if (dinit->filename != NULL) snprintf(fn, sizeof(fn), "Editing %s, x = %d", dinit->filename, dinit->d.cursorX);
    else snprintf(fn, sizeof(fn), "Editing Untitled, x = %d", dinit->d.cursorX - 6);
    if (dinit->modified > 0) {
        strncat(fn, " (modified)", strlen(fn) + 11);
    }
    for (int i = 0; i < dinit->height; i++) {
        if (i == 20) {
            pos(i, dinit->height, a);
            app(strlen(fn), fn, a);
        }
        else {
            app(1, " ", a);
        }
    }
    reset(a);
    pos(dinit->d.cursorX, dinit->d.cursorY, a);
}

// get window size
void getWindowSize(DisplayInit* dinit) {
    struct winsize w;
    int f = open("/dev/tty", O_RDWR);

    if (f < 0 || ioctl(f, TIOCGWINSZ, &w) < 0) errorHandle("open");
    dinit->width = w.ws_row + 1;
    dinit->height = w.ws_col + 1;
}

// move the cursor to the required coordinates
void pos(int x, int y, App* a) {
    char retArr[64];
    snprintf(retArr, sizeof(retArr), "\033[%d;%dH", (y), (x));
    app(strlen(retArr), retArr, a);
}

// displays the keys
char displayKeys() {
    char character = '\0';
    if (read(STDIN_FILENO, &character, 1) == -1 && errno != EAGAIN)
        exit(1);

    if (character == '\033') {
        char arrKey[3];
        if (read(STDIN_FILENO, &arrKey[0], 1) != -1) character = '\033';
        if (read(STDIN_FILENO, &arrKey[1], 1) != -1) character = '\033';

        // checks for arrow keys, then sets the character to the
        // appropiate one.
        if (arrKey[0] == '[') {
            switch (arrKey[1]) {
                case 'A':
                    character = ARROW_UP;
                    break;
                case 'B':
                    character = ARROW_DOWN;
                    break;
                case 'C':
                    character = ARROW_RIGHT;
                    break;
                case 'D':
                    character = ARROW_LEFT;
                    break;
            }
        }
    }

    return character;
}

// clear display
void clearDisplay(App* a) {
    app(2, "\033c", a);
}

// function to append messages
void app(int length, char* string, App* a) {
    char* newStr = realloc(a->string, a->length + length);
    
    if (newStr == NULL) return; // fail
    memcpy(&newStr[a->length], string, length);
    a->string = newStr;
    a->length += length;
}

// display a message to the user
void systemShowMessage(DisplayInit* dinit, App* a) {
    if (time(NULL) <= dinit->m.length + MSG_WAIT_TIMES) {
        pos(0, dinit->width - 2, a);
        app(strlen(dinit->m.msg), dinit->m.msg, a);
    }
    else {
        memset(dinit->m.msg, 0, strlen(dinit->m.msg));
    }
}

// allow the user to input certain things
char *systemScanfUser(DisplayInit* dinit, char* msg) {
    char* strmsg = NULL;
    char* screenMsg = NULL;
    int len = 1;
    strmsg = malloc(len);
    strmsg[0] = '\0';

    // store current positions
    int x = dinit->d.cursorX;
    int y = dinit->d.cursorY;

    while (1) {
        screenMsg = malloc(strlen(msg) + len + 17); // 17 is the size of the first string
        snprintf(screenMsg, strlen(msg) + len + 17, "Ctrl+C: Cancel | %s%s", msg, strmsg);
        setDinitMsg(dinit, screenMsg, strlen(screenMsg));
        dinit->d.cursorX = x;
        dinit->d.cursorY = y;

        char c = displayKeys();
        if (c == '\n' || c == '\r') {
            return strmsg;
        }
        else if (c == CTRL_KEY('c')) {
            return 0;
        }
        else if (c == 127) { // 127 is backspace
            if (len > 1) {
                strmsg[len - 1] = '\0';
                len--;
            }
        }
        if (!iscntrl(c)) {
            strmsg = realloc(strmsg, len + 1);
            strmsg[len - 1] = c;
            strmsg[len] = '\0';
            len++;
        }
        dinit->d.cursorX = strlen(screenMsg) + 1;
        dinit->d.cursorY = dinit->width - 2;
        bufferDisplay(dinit);
    }
}

// set the display message to anything we want
void setDinitMsg(DisplayInit* dinit, char* msg, int length) {
    dinit->m.msg = malloc(length + 1);
    memcpy(dinit->m.msg, msg, length);
    dinit->m.msg[length] = '\0';
    dinit->m.length = time(NULL);
}

// show linenumbers on the screen
void lineNumShow(App* a, DisplayInit* dinit) {    
    // disable line wrapping
    app(5, "\033[?7l", a);
    int showLine = 1 + dinit->d.scrollY;

    while (showLine - dinit->d.scrollY < dinit->width - 1) {
        Row* row = &dinit->row[showLine - 1];
        // display the linenum
        char Num[32];
        snprintf(Num, sizeof(Num), "%d", showLine);

        /*
            we want the background to be dark grey
            additionally, we also want the foreground to be a 
            lighter grey, unless if the cursor is on the line
        */
        processBG(a, DARKER_GREY);
        if (dinit->d.cursorY + dinit->d.scrollY == showLine) processFG(a, WHITE);
        else processFG(a, LIGHTER_GREY);

        // make sure its on the right side of the linenum
        pos(0, showLine - dinit->d.scrollY, a);
            
        for (int y = 0; y < (6 - (strlen(Num)) - 1); y++) {
            app(1, " ", a);
        }

        if (showLine <= dinit->linenum) {
            pos((6 - (strlen(Num)) - 1), showLine - dinit->d.scrollY, a);
            app(strlen(Num), Num, a);
            pos(5, showLine - dinit->d.scrollY, a);
            app(1, " ", a);
            processFG(a, WHITE);

            // append the text in
            int length = row->tabs.tlen;
            app(length, row->tabs.tab, a);
        }

        showLine++;
        app(3, "\x1b[K", a);
    }
    reset(a);
}

// buffer 
void bufferDisplay(DisplayInit* dinit) {
    App a = {NULL, 0};
    // reset display
    clearDisplay(&a);

    // show lines
    lineNumShow(&a, dinit);

    // show mesage
    systemShowMessage(dinit, &a);

    // show status bar
    drawStatusBar(&a, dinit);
    
    // write everything to the terminal
    write(STDOUT_FILENO, a.string, a.length);
    free(a.string);
}

// init
void showDisplay(DisplayInit* dinit) {
    dinit->d.cursorX = 6;
    dinit->d.cursorY = 1;
    dinit->d.offsetX = 0;
    dinit->d.calculateLengthStop = dinit->row[0].length;
    dinit->modified = 0;
    dinit->d.scrollX = 0;
    dinit->d.scrollY = 0;
    setDinitMsg(dinit, "", 0);
    getWindowSize(dinit);

    while (1) {
        bufferDisplay(dinit);
        
        // get and process the keypresses
        processKeypresses(displayKeys(), dinit);
    }
}