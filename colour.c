#include "colour.h"

#include <stdio.h>
#include <string.h>

// process background colours
void processBG(App* a, int colour) {
    char process[12];
    snprintf(process, sizeof(process), "\033[48;5;%dm", colour);
    app(strlen(process), process, a);
}

// process foreground colours
void processFG(App* a, int colour) {
    char process[12];
    snprintf(process, sizeof(process), "\033[38;5;%dm", colour);
    app(strlen(process), process, a);
}

// reset background to normal
void reset(App* a) {
    app(3, "\033[m", a);
}