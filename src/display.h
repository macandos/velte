#ifndef DISPLAY_H_
#define DISPLAY_H_ 

#include <stdint.h>
#include <stdarg.h>

#include "structs.h"

int displayDraw(App* a, const char* format, ...);
void writeToAppendBuffer(size_t fLen, char* formattedString, App* a);
void writeDisplay(App* a);
void clearDisplay(App* a); 
void showDisplay(Editor* editor); 
void bufferDisplay(Editor* editor);
void lineNumShow(Editor* editor);
void controlKeypresses(Editor* editor);
void pos(size_t x, int y, App* a);
void systemShowMessage(Editor* editor); 
void getWindowSize(Editor* editor); 
void drawStatusBar(Editor* editor);
char *editorPrompt(Editor* editor, char* msg);
void seteditorMsg(Editor* editor, char* msg, size_t length);
void initConfig(Editor* editor);
void* check_malloc(size_t length);
void* check_realloc(void* buff, size_t length);

#endif
