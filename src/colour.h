#ifndef COLOUR_H_
#define COLOUR_H_

#include "structs.h"
#include "display.h"

#define COLOUR(R, G, B) { .r = R, .g = G, .b = B };

void processBG(App* a, Rgb colour);
void processFG(App* a, Rgb colour);
void reset(App* a);

#endif