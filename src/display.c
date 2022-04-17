#include "display.h"
#include "error.h"
#include "keypresses.h"
#include "colour.h"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define MSG_WAIT_TIMES 30

// draw the status bar
void drawStatusBar(App* a, DisplayInit* dinit) {
    processBG(a, DARK_GREY);
    processFG(a, LIGHTER_GREY);
    
    app(24, "The Velte Text Editor  ", a);
    char dis[40];

    // display the current line and char
    char c = dinit->row[dinit->d.cursorY - 1].line[dinit->d.cursorX - 6];

    if (!iscntrl(c))
        snprintf(dis, sizeof(dis), "Current char: %c - Current line: %d", c, dinit->d.cursorY);
    else
        snprintf(dis, sizeof(dis), "Current char: %c - Current line: %d", ' ', dinit->d.cursorY);

    char fn[64];
    if (dinit->filename != NULL) 
        snprintf(fn, sizeof(fn), "Editing %s", dinit->filename);
    else
        snprintf(fn, sizeof(fn), "Editing Untitled");

    if (dinit->modified > 0) {
        strncat(fn, " (modified)", strlen(fn) + 11);
    }
    for (int i = 0; i < dinit->height; i++) {
        if (i == 25) {
            pos(i, dinit->height, a);
            app(strlen(fn), fn, a);
        }
        else if (i == dinit->height - strlen(dis)) {
            pos(i, dinit->height, a);
            app(strlen(dis), dis, a);
            break;
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
void displayKeys(DisplayInit* dinit) {
    char character;

    if (read(STDIN_FILENO, &character, 1) == -1 && errno != EAGAIN)
        errorHandle("read");

    if (character == '\033') {
        char arrKey[3];
        if (read(STDIN_FILENO, &arrKey[0], 1) != -1) character = '\033';
        if (read(STDIN_FILENO, &arrKey[1], 1) != -1) character = '\033';

        // checks for arrow keys
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

    processKeypresses(character, dinit);
}

// clear display
void clearDisplay(App* a) {
    app(2, "\033c", a);
}

void aF(App* a) {
    free(a->string);
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
void systemShowMessage(App* a, char msg[], DisplayInit* dinit) {
    static int wait = MSG_WAIT_TIMES;
    if (wait > 0 && dinit->msg[0] != '\0') {
        pos(0, dinit->width - 2, a);

        for (int i = 0; i < dinit->height; i++) {
            if (i == (dinit->height / 2) - strlen(msg)) {
                app(strlen(msg), msg, a);
            }
            app(1, " ", a);
        }
        pos(dinit->d.cursorX, dinit->d.cursorY, a);
    }
    else if (wait == 0) {
        memset(dinit->msg, 0, strlen(msg));
        wait = MSG_WAIT_TIMES;
    }
    wait--;
}

// show linenumbers on the screen
void lineNumShow(App* a, DisplayInit* dinit) {    
    // disable line wrapping
    app(5, "\033[?7l", a);
    int showLine = 1;

    while (showLine < dinit->height - 1) {
        // display the linenum
        char Num[32];
        snprintf(Num, sizeof(Num), "%d", showLine);

        /*
            we want the background to be dark grey
            additionally, we also want the foreground to be a 
            lighter grey
        */
        processBG(a, DARKER_GREY);
        processFG(a, LIGHTER_GREY);

        // make sure its on the right side of the linenum
        pos(0, showLine, a);
            
        for (int y = 0; y < (6 - (strlen(Num)) - 1); y++) {
            app(1, " ", a);
        }

        if (showLine <= dinit->linenum) {
            pos((6 - (strlen(Num)) - 1), showLine, a);
            app(strlen(Num), Num, a);

            pos(5, showLine, a);
            app(1, " ", a);

            // append the text in
            int length = dinit->row[showLine - 1].length;
            app(length, dinit->row[showLine - 1].line, a);
        }
        else {
            pos(4, showLine, a);
            app(1, "-", a);

            for (int y = 0; y < dinit->height; y++) {
                app(1, " ", a);
            }
        }

        showLine++;
        app(3, "\x1b[K", a);
    }

    reset(a);
}

// buffer 
void bufferDisplay(DisplayInit* dinit) {
    App a = APP_INIT;

    // reset display
    clearDisplay(&a);

    // show lines
    lineNumShow(&a, dinit);

    // show status bar
    pos(0, dinit->height, &a);
    drawStatusBar(&a, dinit);

    // display keys
    displayKeys(dinit);

    // show the display message to the user
    systemShowMessage(&a, dinit->msg, dinit);
    
    // write everything to the terminal
    write(STDOUT_FILENO, a.string, a.length);
    aF(&a);
}

// init
void showDisplay(DisplayInit* dinit) { 
    dinit->numbersize = 1;
    dinit->d.cursorX = 6;
    dinit->d.cursorY = 1;
    dinit->d.scrollY = 1;
    dinit->modified = 0;
    dinit->msg[0] = '\0';

    while (1) {
        getWindowSize(dinit);
        bufferDisplay(dinit);
    }
}