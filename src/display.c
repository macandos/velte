#include "display.h"
#include "error.h"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>

Dimensions d;
DisplayInit dinit;

// move the cursor to the required coordinates
void pos(int x, int y, App* a) {
    char retArr[64];
    snprintf(retArr, sizeof(retArr), "\033[%d;%dH", (y), (x));
    app(strlen(retArr), retArr, a);
}

// displays the keys
void displayKeys(char character, App* a) {
    if (read(STDIN_FILENO, &character, 1) == -1 && errno != EAGAIN)
        errorHandle("read");

    if (character == 'e') errorHandle("end");

    if (!iscntrl(character)) {
        //pos(d.x, d.y, a);

        //d.x++;
    }
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
    int i;
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    pos(d.x, d.y, a);
    
    // disable line wrapping
    app(5, "\033[?7l", a);

    for (i = 0; i <= dinit.numbersize; i++) {
        // so we can display the linenum
        char charNum[32];
        snprintf(charNum, sizeof(charNum), "%d", (i + 1));

        if (i >= dinit.linenum) {
            app(strlen(charNum), charNum, a); 
            pos(6, d.y, a);

            if (i < w.ws_col - 1) {
                app(strlen(charNum), "\r\n", a);
            }  
        }
        else {
            app(strlen(charNum), charNum, a); 
            pos(6, d.y, a);

            app(dinit.row[i].length, dinit.row[i].line, a);

            if (dinit.numbersize <= (dinit.linenum + 1)) {
                dinit.numbersize++;
            }
            d.y++;            
        }
        
        app(3, "\x1b[K", a);
    }

    pos(6, d.y, a);
}

// buffer 
void bufferDisplay() {
    App a = APP_INIT;
    char character;
    d.y = 1;

    // reset display
    clearDisplay(&a);

    // show lines
    lineNumShow(&a);

    // displeys
    displayKeys(character, &a);

    write(STDOUT_FILENO, a.string, a.length);
    aF(&a);
}

// init
void showDisplay(DisplayInit din) { 
    d.x = 0;
    d.y = 0;
    dinit = din;
    dinit.numbersize = 1;

    while (1) {
        bufferDisplay();
    }
}

