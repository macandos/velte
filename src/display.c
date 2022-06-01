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
    
    app(20, "Velte Text Editor  ", a);
    char dis[40];

    // display the current line and char
    char c = dinit->row[dinit->d.cursorY - 1].line[dinit->d.tabX - 7];
    if (!iscntrl(c)) snprintf(dis, sizeof(dis), "Char: %c - Line: %d", c, dinit->d.cursorY);
    else if (c == '\t') snprintf(dis, sizeof(dis), "Char: Tab");
    else snprintf(dis, sizeof(dis), "Char: %c - Line: %d", ' ', dinit->d.cursorY);

    // append the filaneme; if any
    // if its null, then just say 'Untitled'
    char fn[64];
    if (dinit->filename != NULL) snprintf(fn, sizeof(fn), "Editing %s", dinit->filename);
    else snprintf(fn, sizeof(fn), "Editing Untitled, x = %d, off = %d, tabx = %d, isTab? = %d", dinit->d.cursorX - 6, dinit->d.offsetX, dinit->d.tabX - 6, isTab(dinit, dinit->d.tabX - 7, dinit->d.cursorY - 1) == 0 ? 0 : 1);
    if (dinit->modified > 0) {
        strncat(fn, " (modified)", strlen(fn) + 11);
    }
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
        errorHandle("read");
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

// we want the user to be able to input information
char *systemScanfUser(App* a, char msg[], DisplayInit* dinit) {
    /* we create a dynamically allocated array */
    size_t len = 32;
    char* buff = (char*)calloc(len, sizeof(char));
    buff[0] = '\0';
    size_t posi = 0;

    while (1) {
        pos(0, dinit->width - 2, a);
        char dMsg[len + strlen(msg)];
        snprintf(dMsg, sizeof(dMsg), "%s%s", msg, buff);
        bufferDisplay(dinit);
        app(strlen(dMsg), dMsg, a);

        char c = displayKeys();
        if (c == '\n' || c == '\r') {
            return buff;
        }
        else if (c < len) {  
            if (!iscntrl(c)) {
                if (posi == len - 1) {
                    len *= 2;
                    buff = realloc(buff, sizeof(char) * len);
                }
                else if (c != '\0') {
                    buff[posi] = c;
                    buff[posi++] = '\0';
                }
            }
        }
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
    App a = APP_INIT;
    // reset display
    clearDisplay(&a);

    // show lines
    lineNumShow(&a, dinit);

    // show the display message to the user
    systemShowMessage(&a, dinit->msg, dinit);

    // show status bar
    pos(0, dinit->height, &a);
    drawStatusBar(&a, dinit);

    // input keypresses
    char c = displayKeys();

    // process them
    processKeypresses(c, dinit, &a);
    
    // write everything to the terminal
    write(STDOUT_FILENO, a.string, a.length);
    free(a.string);
}

// init
void showDisplay(DisplayInit* dinit) {
    dinit->d.cursorX = 6;
    dinit->d.cursorY = 1;
    dinit->d.offsetX = 0;
    dinit->d.tabX = 6;
    dinit->modified = 0;
    dinit->msg[0] = '\0';
    getWindowSize(dinit);

    while (1) {
        bufferDisplay(dinit);
    }
}