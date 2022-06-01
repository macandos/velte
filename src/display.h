#ifndef DISPLAY_H_
#define DISPLAY_H_ 
 
#include <sys/types.h> 
 
typedef struct { 
    // sees what pos the cursor is on 
    int cursorX;
    int offsetX;
    int tabX;
    int cursorY; 
} Dimensions; 

typedef struct {
    char* tab;
    int tlen;
} TabsInit;
 
typedef struct { 
    char* string; 
    int length; 
} App; 

typedef struct { 
    int length; 
    char* line;

    // tabs
    TabsInit tabs;
} Row; 
 
typedef struct { 
    int linenum;  
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

    // display a status message
    char msg[64];
} DisplayInit; 
 
#define APP_INIT {NULL, 0} 
 
void clearDisplay(App* a); 
void showDisplay(DisplayInit* dinit); 
void bufferDisplay(DisplayInit* dinit);
void lineNumShow(App* a, DisplayInit* dinit); 
void app(int length, char* string, App* a);  
char displayKeys(); 
void pos(int x, int y, App* a);
void systemShowMessage(App* a, char msg[], DisplayInit* dinit); 
void getWindowSize(DisplayInit* dinit); 
void drawStatusBar(App* a, DisplayInit* dinit);
char *systemScanfUser(App* a, char msg[], DisplayInit* dinit);

 
#endif
