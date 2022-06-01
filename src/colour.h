#ifndef COLOUR_H_
#define COLOUR_H_

#include "display.h"

// colours
// GREYSCALE
#define DARK_GREY 235
#define DARKER_GREY 234
#define LIGHT_GREY 237
#define LIGHTER_GREY 245
#define WHITE 255

void processBG(App* a, int colour);
void processFG(App* a, int colour);
void reset(App* a);

#endif