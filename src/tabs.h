#ifndef TABS_H_
#define TABS_H_

#include "display.h"

void tabChange(DisplayInit* dinit, int pos);
void cursorMovementTab(DisplayInit* dinit, char c);
int isTab(DisplayInit* dinit, int pos, int y);
int getTabCount(DisplayInit* dinit, int y);
void controlOffsetX(DisplayInit* dinit);

#endif