#include "raw.h"
#include "error.h"

#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>

struct termios term;

// if exit, restore the terminal's original settings
void endRaw() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) == -1) 
        errorHandle("tcsetattr");

    // reset position
    //printf("\033[0;0H");
    printf("\033c");
}

// used to read input one-at-a-time, instead of all at one line
void raw() {
    // kill program when something fails
    if (tcgetattr(STDIN_FILENO, &term) == -1)
        errorHandle("tcgetattr");

    atexit(endRaw);

    struct termios rawS = term;

    // disable a bunch of flags
    rawS.c_lflag &= ~(ECHO | ISIG | IXON | ICANON | ICRNL | IEXTEN);
    rawS.c_lflag &= ~(BRKINT | CS8 | INPCK | ISTRIP);

    rawS.c_cc[VMIN] = 0;
    rawS.c_cc[VTIME] = 1;
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawS) == -1) 
        errorHandle("tcsetattr");
}