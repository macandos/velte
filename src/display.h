#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <sys/types.h>

typedef struct {
    int x;
    int y;

    // sees what pos the cursor is on
    int cursorX;
    int cursorY;

    // for scrolling purposes
    //int scrollY;
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

    // cursor pos
    Dimensions d;

    // terminal size
    int width;
    int height;

    // get filename
    char* filename;

    // check if the file was modified or not
    int modified;
} DisplayInit;

#define APP_INIT {NULL, 0}

void clearDisplay(App* a);
void showDisplay(DisplayInit din);
void bufferDisplay();
void lineNumShow(App* a);
void app(int length, char* string, App* a);
void insertCharacter(char character);
void displayKeys();
void pos(int x, int y, App* a);
void getWindowSize();
void drawStatusBar(App* a);

#endif /* DISPLAY_H_ */