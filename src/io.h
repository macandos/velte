#ifndef IO_H_
#define IO_H_

#include "structs.h"

void openFile(char* filename, Editor* editor);
off_t getFileLength(const char* filename);
void createRow(int pos, Editor* editor);
void changeStr(uint32_t* str, size_t length, int pos, Editor* editor);
void appendRow(uint32_t* str, size_t length, int pos, Editor* editor);
int handleUnwantedChars(const char* text, size_t length);
void writeFile(Editor* editor);

#endif
  