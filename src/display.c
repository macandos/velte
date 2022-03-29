#include "display.h"
#include "error.h"
#include "keypresses.h"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

DisplayInit dinit;

// draw the status bar
void drawStatusBar(App* a) {
    app(4, "\033[7m", a);
    app(25, "The Velte Text Editor - ", a);
    char dis[40];

    // display the current line and char
    char c = dinit.row[dinit.d.cursorY - 1].line[dinit.d.cursorX - 6];

    if (!iscntrl(c))
        snprintf(dis, sizeof(dis), "Current char: %c - Current line: %d", c, dinit.d.cursorY);
    else
        snprintf(dis, sizeof(dis), "Current char: %c - Current line: %d", ' ', dinit.d.cursorY);

    char fn[64];
    if (dinit.filename != NULL) 
        snprintf(fn, sizeof(fn), "Editing %s", dinit.filename);
    else
        snprintf(fn, sizeof(fn), "Editing Untitled");

    if (dinit.modified > 0) {
        strncat(fn, " (modified)", strlen(fn) + 11);
    }
    for (int i = 0; i < dinit.height; i++) {
        if (i == 25) {
            pos(i, dinit.height, a);
            app(strlen(fn), fn, a);
        }
        else if (i == dinit.height - strlen(dis)) {
            pos(i, dinit.height, a);
            app(strlen(dis), dis, a);
            break;
        }
        else {
            app(1, " ", a);
        }
    }
    app(3, "\033[m", a);
    pos(dinit.d.cursorX, dinit.d.cursorY, a);
}

// get window size
void getWindowSize() {
    struct winsize w;
    int f = open("/dev/tty", O_RDWR);

    if (f < 0 || ioctl(f, TIOCGWINSZ, &w) < 0) errorHandle("open");
    dinit.width = w.ws_row + 1;
    dinit.height = w.ws_col + 1;
}

// move the cursor to the required coordinates
void pos(int x, int y, App* a) {
    char retArr[64];
    snprintf(retArr, sizeof(retArr), "\033[%d;%dH", (y), (x));
    app(strlen(retArr), retArr, a);
}

// displays the keys
void displayKeys() {
    char character = '\0';

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

    dinit = processKeypresses(character, dinit);
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

// show linenumbers on the screen
void lineNumShow(App* a) {
    pos(dinit.d.x, dinit.d.y, a);
    Row* row = &dinit.row[dinit.d.cursorY - 1]; 
    
    // disable line wrapping
    app(5, "\033[?7l", a);

    for (int i = 0; i <= dinit.width; i++) {
        // so we can display the linenum
        char charNum[32];
        snprintf(charNum, sizeof(charNum), "%d", (i + 1));

        if (i >= dinit.linenum);
            // do nothing
        else {
            app(strlen(charNum), charNum, a); 
            pos(6, dinit.d.y, a);

            int length = dinit.row[i].length;
            app(length, dinit.row[i].line, a);
            if (dinit.numbersize <= (dinit.linenum + 1)) {
                dinit.numbersize++;
            }
            dinit.d.y++;   

            // so the linenum wouldnt go onto the prev. line
            if (row->line[row->length] != '\n')
                row->line[row->length] = '\n';
        }
        
        app(3, "\x1b[K", a);
    }
}

// buffer 
void bufferDisplay() {
    App a = APP_INIT;
    char character;
    dinit.d.y = 1;

    // reset display
    clearDisplay(&a);

    // show lines
    lineNumShow(&a);

    // show status bar
    pos(0, dinit.height, &a);
    drawStatusBar(&a);

    // display keyss
    displayKeys();

    // write everything to the terminal
    write(STDOUT_FILENO, a.string, a.length);
    aF(&a);
}

// init
void showDisplay(DisplayInit din) { 
    // init
    dinit = din;
    dinit.numbersize = 1;
    dinit.d.x = 0;
    dinit.d.y = 0;
    dinit.d.cursorX = 6;
    dinit.d.cursorY = 1;
    dinit.modified = 0;

    while (1) {
        getWindowSize();
        bufferDisplay();
    }
}