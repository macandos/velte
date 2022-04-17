#ifndef IO_H_
#define IO_H_

#include "display.h"

void openFile(char* filename, DisplayInit *dinit);
void createRow(char* text, int length, int pos, DisplayInit* dinit);
int handleUnwantedChars(const char* text, ssize_t length);
void writeFile(DisplayInit* dinit);

#endif