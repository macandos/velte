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
#include <string.h>
#include <fcntl.h>

#define MSG_WAIT_TIMES 30

// draw the status bar
void drawStatusBar(App* a, DisplayInit* dinit) {
    processBG(a, DARK_GREY);
    processFG(a, WHITE);
    
    app(18, "Velte Text Editor, ", a);
    char dis[40];

    // display the current line and char
    char c = dinit->row[dinit->d.cursorY - 1].line[dinit->d.cursorX - 6];
    if (!iscntrl(c)) snprintf(dis, sizeof(dis), "Char: %c - Line: %d", c, dinit->d.cursorY);
    else if (c == '\t') snprintf(dis, sizeof(dis), "Char: Tab");
    else snprintf(dis, sizeof(dis), "Char: %c - Line: %d", ' ', dinit->d.cursorY);

    // append the filaneme; if any
    // if its null, then just say 'Untitled'
    char fn[64];
    if (dinit->filename != NULL) snprintf(fn, sizeof(fn), "Editing %s, x = %d", dinit->filename, dinit->d.cursorX - 6);
    else snprintf(fn, sizeof(fn), "Editing Untitled, x = %d", dinit->d.cursorX - 6);
    // if (dinit->modified > 0) {
    //     strncat(fn, " (modified)", strlen(fn) + 11);
    // }
    for (int i = 0; i < dinit->height; i++) {
        if (i == 20) {
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
void systemShowMessage(App* a, DisplayInit* dinit) {
    static int wait = MSG_WAIT_TIMES;
    if (wait > 0 && dinit->msg[0] != '\0') {
        pos(0, dinit->width - 2, a);

        for (int i = 0; i < dinit->height; i++) {
            if (i == (dinit->height / 2) - strlen(dinit->msg)) {
                app(strlen(dinit->msg), dinit->msg, a);
            }
            app(1, " ", a);
        }
        wait--;
    }
    else if (wait == 0) {
        memset(dinit->msg, 0, strlen(dinit->msg));
        wait = MSG_WAIT_TIMES;
    }
}

// show linenumbers on the screen
void lineNumShow(App* a, DisplayInit* dinit) {    
    // disable line wrapping
    app(5, "\033[?7l", a);
    int showLine = 1;

    while (showLine < dinit->height - 1) {
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
        if (dinit->d.cursorY == showLine) processFG(a, WHITE);
        else processFG(a, LIGHTER_GREY);

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

    //show the display message to the user
    systemShowMessage(&a, dinit);

    // show status bar
    pos(0, dinit->height, &a);
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
    dinit->d.calculateLengthStop = 1;
    dinit->modified = 0;
    dinit->msg[0] = '\0';
    getWindowSize(dinit);

    while (1) {
        bufferDisplay(dinit);
        
        // get and process the keypresses
       processKeypresses(displayKeys(), dinit);
    }
}