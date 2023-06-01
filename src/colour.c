#include "colour.h"

#include <stdio.h>
#include <string.h>

// process background colours
void processBG(App* a, Rgb colour) {
    if (colour.isTransparent) {
        return;
    }
    displayDraw(a, "\033[48;2;%d;%d;%dm", colour.r, colour.g, colour.b);
}

// process foreground colours
void processFG(App* a, Rgb colour) {
    // in this case, just set it to the default (white)
    if (colour.isTransparent == true) {
        displayDraw(a, "\033[38;2;%d;%d;%dm", 255, 255, 255);
        return;
    }
    displayDraw(a, "\033[38;2;%d;%d;%dm", colour.r, colour.g, colour.b);
}

// reset background to normal
void reset(App* a) {
    displayDraw(a, "\033[m");
}
