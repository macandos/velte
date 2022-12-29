#ifndef STRUCTS_H_
#define STRUCTS_H_

#include <sys/types.h>
#include <stdint.h>
 
typedef enum {
    ARROW_UP = 1,
    ARROW_DOWN = 2,
    ARROW_RIGHT = 3,
    ARROW_LEFT = 4
} ArrowKeys;

typedef struct {
    unsigned char tabcount;
    unsigned int disLine;
    int isUtf8;
} Config;

typedef struct {
    size_t cursorX;
    size_t maskX;
    size_t tabX;
    int cursorY;
    int posY;
    size_t scrollX;
    int scrollY;
    int utfJump;
} Cursor;

typedef struct {
    size_t length;
    uint32_t* str;
} Buffer;

typedef struct {
    uint32_t* tab;
    size_t tlen;
} Tabs;

typedef struct {
    char* msg;
    int time;
    char isStatus;
} Msg;

typedef struct { 
    char* string; 
    size_t length;
} App; 

typedef struct {
    uint32_t* str;
    size_t length;
    Tabs tabs;
} Row;

typedef struct { 
    int linenum;
    Row* row;
    Cursor c;
    Config config;
    App a;
    int width; 
    int height; 
    char* filename; 
    int modified;
    int tabRem;
    Msg m;
} Editor;

#endif