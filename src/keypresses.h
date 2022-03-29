#ifndef KEYPRESSES_H_
#define KEYPRESSES_H_

#include "display.h"

typedef enum {
    ARROW_UP = 1,
    ARROW_DOWN = 2,
    ARROW_RIGHT = 3,
    ARROW_LEFT = 4
} arrowKeys;

DisplayInit processKeypresses(char character, DisplayInit dinit);
void appChar(DisplayInit* dinit, int pos, char character);
void checkCursor(DisplayInit* dinit, char c);
void delChar(DisplayInit* dinit, int pos);

#endif /* KEYPRESSES_H_ */