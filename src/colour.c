#include "colour.h"

#include <stdio.h>
#include <string.h>

// process background colours
void processBG(App* a, int colour) {
    displayDraw(a, "\033[48;5;%dm", colour);
}

// process foreground colours
void processFG(App* a, int colour) {
    displayDraw(a, "\033[38;5;%dm", colour);
}

// reset background to normal
void reset(App* a) {
    displayDraw(a, "\033m");
}
