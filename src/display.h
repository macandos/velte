#ifndef DISPLAY_H_
#define DISPLAY_H_ 
 
#include <sys/types.h> 
 
typedef struct { 
    // sees what pos the cursor is on 
    int cursorX;
    int offsetX;
    int tabX;
    int cursorY;
    int calculateLengthStop;

    // scrolling
    int scrollX;
    int scrollY;
} Dimensions; 

typedef struct {
    char* tab;
    int tlen;
} TabsInit;

typedef struct {
    char* msg;
    int length;
} MsgInit;

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

    // display status message
    MsgInit m;
} DisplayInit; 
 
void clearDisplay(App* a); 
void showDisplay(DisplayInit* dinit); 
void bufferDisplay(DisplayInit* dinit);
void lineNumShow(App* a, DisplayInit* dinit); 
void app(int length, char* string, App* a);  
char displayKeys(); 
void pos(int x, int y, App* a);
void systemShowMessage(DisplayInit* dinit, App* a); 
void getWindowSize(DisplayInit* dinit); 
void drawStatusBar(App* a, DisplayInit* dinit);
char *systemScanfUser(DisplayInit* dinit, char* msg);
void setDinitMsg(DisplayInit* dinit, char* msg, int length);

 
#endif
