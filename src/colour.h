#ifndef COLOUR_H_
#define COLOUR_H_

#include "structs.h"
#include "display.h"

#define COLOUR(R, G, B) { .r = R, .g = G, .b = B};
#define IS_EQUAL(A, B) (A.r == B.r && A.b == B.b && A.g == B.g)

void processBG(App* a, Rgb colour);
void processFG(App* a, Rgb colour);
void reset(App* a);

#endif