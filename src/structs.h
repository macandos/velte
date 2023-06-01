#ifndef STRUCTS_H_
#define STRUCTS_H_

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>


typedef enum {
    ARROW_UP = 1,
    ARROW_DOWN,
    ARROW_RIGHT,
    ARROW_LEFT
} ArrowKeys;

typedef enum {
    LINENUM_TEXT_COLOUR,
    LINENUM_TEXT_ACTIVE_COLOUR,
    LINENUM_COLOUR,
    STATUS_TEXT_COLOUR,
    STATUS_COLOUR,
    BACKGROUND_COLOUR,
    MESSAGE_COLOUR,
    COLOUR_CONFIG_LENGTH
} ColourConfig;

#define DEFAULT_DISLINE 6
#define DEFAULT_TABCOUNT 4

typedef struct {
    unsigned char r, g, b;
    bool isTransparent;
} Rgb;

typedef struct {
    size_t tabcount;
    size_t disLine;
    bool isUtf8;
    bool linenums;
    Rgb* colours;
} Config;

typedef struct {
    size_t cursorX;
    size_t tabX;
    size_t cursorY;
    int scrollX;
    int scrollY;
    int utfJump;
} Cursor;

typedef struct {
    uint32_t* tab;
    size_t tlen;
} Tabs;

typedef struct {
    char* msg;
    int time;
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