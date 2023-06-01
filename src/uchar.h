#ifndef UCHAR_H_
#define UCHAR_H_

#include <stdint.h>
#include <stdlib.h>
#include "structs.h"

uint32_t* mbtoutf(const char* str, size_t length, size_t* arrLen);
char* utftomb(const uint32_t* str, size_t length, size_t* arrLen);
uint32_t chartouint(const char* str);
char getCharWidth(uint32_t ch);
uint32_t utfToCodepoint(const char* in, char chLen);
void calculateCharacterWidth(Editor* editor, uint32_t ch, int* utfJump);
uint32_t getMKeys();
char getLenUTF(uint32_t c);
int isValidUTF(uint32_t character);
uint32_t reverseBytes(uint32_t character, char length);

#endif