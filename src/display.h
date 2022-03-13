#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <sys/types.h>

typedef struct {
    int x;
    int y;
} Dimensions;

typedef struct {
    char* string;
    int length;
} App;

typedef struct {
    int length;
    char* line;
} Row;

typedef struct {
    int linenum;
    int numbersize;
    Row* row;
} DisplayInit;

#define APP_INIT {NULL, 0}

void clearDisplay(App* a);
void showDisplay(DisplayInit din);
void bufferDisplay();
void lineNumShow(App* a);
void app(int length, char* string, App* a);
void insertCharacter(char character);
void displayKeys(char character, App* a);
void pos(int x, int y, App* a);
void insertFileByLine(App* a);

#endif /* DISPLAY_H_ */