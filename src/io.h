#ifndef IO_H_
#define IO_H_

#include "display.h"

void openFile(char* filename, DisplayInit *dinit);
void createRow(char* text, size_t length, int pos, DisplayInit* dinit);
void changeStr(char* str, size_t length, int pos, DisplayInit* dinit);
int handleUnwantedChars(const char* text, ssize_t length);
void writeFile(DisplayInit* dinit, App* a);

#endif