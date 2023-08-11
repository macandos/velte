#include "display.h"
#include "error.h"
#include "keypresses.h"
#include "colour.h"
#include "uchar.h"
#include "io.h"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <limits.h>
#include "../libs/uthash.h"

#define MSG_WAIT_TIMES 5
#define STARTING_SAVE_STR 18

// draw the status bar
void drawStatusBar(Editor* editor) {
    pos(0, editor->width, &editor->a);
    processBG(&editor->a, editor->config.colours[STATUS_COLOUR]);
    processFG(&editor->a, editor->config.colours[STATUS_TEXT_COLOUR]);
    
    int len = displayDraw(&editor->a, "Velte - %s %d/%d %s %s",
                            editor->filename ? editor->filename : "Untitled",
                            editor->c.scrollX, editor->c.cursorY,
                            (!editor->modified) ? "-" : "(modified) -",
                            editor->currSyntax->syntaxName);
    for (int i = len; i < editor->height - 1; i++) displayDraw(&editor->a, " ");
    reset(&editor->a);
}

// get window size
void getWindowSize(Editor* editor) {
    struct winsize w;
    int f = open("/dev/tty", O_RDWR);

    if (f < 0 || ioctl(f, TIOCGWINSZ, &w) < 0) return;
    editor->width = w.ws_row + 1;
    editor->height = w.ws_col + 1;
}

// move the cursor to the required coordinates
void pos(size_t x, int y, App* a) {
    displayDraw(a, "\033[%d;%dH", (y), (x));
}

// get input from the user and process it
void controlKeypresses(Editor* editor) {
    uint32_t c = getMKeys();
    processKeypresses(c, editor);
}

// clear display
void clearDisplay(App* a) {
    displayDraw(a, "\033c");
}

// error handling
void errorHandle(const char* err) {
    perror(err);
    exit(1);
}

// malloc and check if it failed so we don't have to do this every time
void* check_malloc(size_t length) {
    void* tmp = malloc(length);
    if (!tmp) {
        errorHandle("Velte: malloc");
    }
    return tmp;
}

// do the same for realloc
void* check_realloc(void* buff, size_t length) {
    void* tmp = realloc(buff, length);
    if (!tmp) {
        errorHandle("Velte: realloc");
    }
    return tmp;
}

// converts a hex string (such as 0F) to a size_t
int editorConvStringToSizeT(Editor* editor, size_t* conv, char* str, int base) {
    char* end;
    long toSizeT = strtol(str, &end, base);
    
    // making sure the value is in range
    if ((unsigned long)toSizeT > SIZE_MAX || toSizeT < 0) {
        seteditorMsg(editor, "Argument is out of range");
        return 0;
    }
    if (*end != '\0') {
        seteditorMsg(editor, "Invalid number");
        return 0;
    }
    *conv = toSizeT;
    return 1;
}

// check if a rgb value is valid and return the converted rgb value
// hexValue must always start with a '#'
int checkRgb(Editor* editor, Rgb* rgbVal, char* hexValue) {
    if (strlen(hexValue) != 7) return 0;
    if (hexValue[0] != '#') return 0;
    char hexCode[3];
    int j = 0;

    for (int i = 1; i < 7; i++) {
        hexCode[j] = hexValue[i];
        j++;
        if (j % 2 == 0) {
            hexCode[2] = '\0';
            size_t value;
            int result = editorConvStringToSizeT(editor, &value, hexCode, 16);
            if (!result || value > UCHAR_MAX) return 0;

            // check what part of the hex code corresponds to each colour, and insert
            // the values into the struct.
            if (i <= 2) rgbVal->r = (unsigned char)value;
            else if (i <= 4) rgbVal->g = (unsigned char)value;
            else rgbVal->b = (unsigned char)value;
            j = 0;
        }
    }
    return 1;
}

// write the string into the append buffer so that it can be displayed all at once to the screen
void writeToAppendBuffer(size_t fLen, char* formattedString, App* a) {
    a->string = check_realloc(a->string, a->length + fLen + 1);
    memcpy(&a->string[a->length], formattedString, fLen);
    a->length += fLen;
    a->string[a->length] = '\0';
}

// draw messages to the screen, and return the length
int displayDraw(App* a, const char* format, ...) {
    va_list ap;
    va_start(ap, format);

    // get the length of the formatted string
    int length = vsnprintf(NULL, 0, format, ap);
    va_end(ap);

    va_start(ap, format);
    char* formattedString = check_malloc(length + 1);
    size_t fLen = vsprintf(formattedString, format, ap);

    writeToAppendBuffer(fLen, formattedString, a);

    va_end(ap);
    free(formattedString);
    return fLen;
}

// write the buffer out and free the buffer
void writeDisplay(App* a) {
    write(STDOUT_FILENO, a->string, a->length);
    free(a->string);
    
    a->string = NULL;
    a->length = 0;
}

// prompts the user to something
char *editorPrompt(Editor* editor, char* msg) {
    uint32_t* strmsg = NULL;
    char* charMsg = NULL;
    char* printMsg = NULL;
    size_t len = 0;
    size_t resultingLen;
    size_t printMsgLen;
    Cursor cursor;
    initCursor(&cursor);
    cursor.cursorY = editor->width - 2;

    strmsg = check_malloc((len + 1) * sizeof(uint32_t));
    strmsg[len] = '\0';

    while (1) {
        // format the message and set it as the editor message
        size_t spliceLen = len - cursor.scrollX;
        charMsg = utftomb(&strmsg[cursor.scrollX], spliceLen, &resultingLen);
        printMsgLen = strlen(msg) + resultingLen + STARTING_SAVE_STR;
        printMsg = check_malloc(printMsgLen + 1);

        int res = snprintf(printMsg, printMsgLen, "Ctrl-X Cancel - %s%s", msg, charMsg);
        if (res < 0) break;
        seteditorMsg(editor, printMsg);
        
        // continue to let the display buffer
        bufferDisplay(editor);
        positionCursor(cursor, strlen(msg) + STARTING_SAVE_STR - 1, &editor->a);
        writeDisplay(&editor->a);
        seteditorMsg(editor, "");

        // get a keypress from the user
        uint32_t c = getMKeys();
        switch (c) {
            case '\n':
            case '\r':
                // return the prompt entered
                return charMsg;
            case CTRL_KEY('x'):
                return 0;
            case BACKSPACE_KEY: {
                if (cursor.cursorX > 0) {
                    size_t prev = cursor.cursorX;
                    memmove(&strmsg[prev - 1], &strmsg[prev], (len - prev) * sizeof(uint32_t));
                    cursor.cursorX--;
                    len--;
                }
                break;
            }
            case ARROW_LEFT:
                if (cursor.cursorX > 0) cursor.cursorX--;
                break;
            case ARROW_RIGHT:
                if (cursor.cursorX < len) cursor.cursorX++;
                break;
            default:
                // TODO: put this into the append char function
                if (c != '\0') {
                    strmsg = check_realloc(strmsg, (len + 2) * sizeof(uint32_t));
                    memmove(&strmsg[cursor.cursorX + 1], &strmsg[cursor.cursorX], (len - cursor.cursorX) * sizeof(uint32_t));
                    strmsg[cursor.cursorX] = c;
                    cursor.cursorX++;
                    len++;

                    strmsg[len] = '\0';
                }
                break;

        }

        // do the same thing we did with the regular horizontal scrolling
        cursor.utfJump = 0;
        for (size_t i = cursor.scrollX; i < cursor.cursorX; i++) {
            // make the mask cursor line up with multi-width characters
            calculateCharacterWidth(editor, strmsg[i], &cursor.utfJump);
        }
        // scroll the message if needed
        scroll(editor, &cursor, strmsg, strlen(msg) + STARTING_SAVE_STR - 1);
    }
    free(strmsg);
    free(charMsg);
    free(printMsg);
    return 0;
}

// set the display message to anything we want
void seteditorMsg(Editor* editor, char* msg) {
    size_t length = strlen(msg);
    editor->msg = check_malloc(length + 1);
    memcpy(editor->msg, msg, length);
    editor->msg[length] = '\0';
    editor->time = time(NULL);
}

// display a message to the user
void systemShowMessage(Editor* editor) {
    if (time(NULL) < editor->time + MSG_WAIT_TIMES) {
        processFG(&editor->a, editor->config.colours[MESSAGE_COLOUR]);
        pos(0, editor->width - 2, &editor->a);
        displayDraw(&editor->a, "%s", editor->msg);
    }
    else {
        // clear it when the time has run out
        free(editor->msg);
        editor->msg = NULL;
    }
}

// check the filename if there is an extension, and set currsyntax to the respected syntax file
int checkExtension(Editor* editor, char* filename) {
    if (filename == NULL) return 0;

    // get the file extension
    char* lastOccurance = strrchr(filename, '.');
    if (!lastOccurance || lastOccurance == filename) return 0;
    lastOccurance++;

    // check if it matches any of our syntax file extensions
    for (int i = 0; i < editor->syntaxLen; i++) {
        if (regexec(&editor->syntaxes[i].fileEndings, lastOccurance, 0, NULL, 0) == 0) {
            editor->currSyntax = (editor->syntaxes + i);
            return 1;
        }
    }
    return 0;
}

// check if the character is a separator
bool isSeparator(char c) {
    if (strchr("!{}:@~<>?,./;'#~[]()-=*+_¬¦\"` \t", c)) {
        return true;
    }
    return false;
}

// initialize a new syntax struct
void createSyntax(Editor* editor, char* pattern, char* name) {
    regex_t regExp;
    // check if the regex is correct
    int ret = regcomp(&regExp, pattern, REG_EXTENDED | REG_NOSUB);
    if (ret != 0) {
        seteditorMsg(editor, "Unable to create syntax: regcomp() failed");
        return;
    }
    // initialize a new syntax field
    int len = editor->syntaxLen;
    int pos = editor->currSyntax - editor->syntaxes;
    editor->syntaxes = check_realloc(editor->syntaxes, (len + 1) * sizeof(Syntax));
    editor->syntaxes[len].map = NULL;
    editor->syntaxes[len].fileEndings = regExp;
    if (strlen(name) < 32) {
        strncpy(editor->syntaxes[len].syntaxName, name, strlen(name));
        editor->syntaxes[len].syntaxName[strlen(name)] = '\0';
    }
    editor->syntaxLen++;

    editor->currSyntax = editor->syntaxes + pos;
}

// append a new syntax rule
SyntaxMap* appendSyntax(Editor* editor, bool isCurr, char* pattern, Rgb colour) {
    SyntaxMap* map = check_malloc(sizeof(SyntaxMap)), *tmp;
    SyntaxMap* currMap = (isCurr ? editor->currSyntax->map : editor->syntaxes[editor->syntaxLen - 1].map);
    regex_t regexExp;

    // if regcomp fails, the error will be stored here
    char failBuff[100];

    // compile the regex. return if it fails
    int ret = regcomp(&regexExp, pattern, REG_EXTENDED);
    if (ret != 0) {
        regerror(ret, &regexExp, failBuff, 100);
        seteditorMsg(editor, failBuff);
        return currMap;
    }

    // if the pattern is already in the list, then return  
    HASH_FIND(hh, currMap, &regexExp, sizeof(regex_t), tmp);
    if (tmp) return currMap;

    // add the values to the hashmap
    map->value = colour;
    map->key = regexExp;
    HASH_ADD(hh, currMap, key, sizeof(SyntaxMap), map);
    return currMap;
}

bool isSyntax(Editor* editor, int* outLen, char* str, size_t pos) {
    SyntaxMap* map, *tmp;
    regmatch_t pmatch[1];
    bool isMatch = false;

    if (!isSeparator(*(str - 1)) && !isSeparator(*(str))) return false;

    // iterate through the hashmap and match
    HASH_ITER(hh, editor->currSyntax->map, map, tmp) {
        if (regexec(&map->key, str, 1, pmatch, 0) == 0) {
            int length = pmatch[0].rm_eo - pmatch[0].rm_so;

            if (!isSeparator(*(str + length)) && !isSeparator(*(str + length - 1))) continue;
            if (length <= 0 || (size_t)pmatch[0].rm_so + pos != pos) continue;

            if (editor->matchLen == 0) {
                *outLen = length;
            }
            if ((size_t)pmatch[0].rm_eo <= (size_t)*outLen) {
                editor->currMatches = check_realloc(editor->currMatches, (editor->matchLen + 1) * sizeof(Match));
                editor->currMatches[editor->matchLen].ends.rm_so = pos;
                editor->currMatches[editor->matchLen].ends.rm_eo = pos + length;
                editor->currMatches[editor->matchLen].colour = map->value;

                editor->currMatchPos = editor->matchLen;
                editor->matchLen++;
            }
            isMatch = true;
        }
    }
    return isMatch;
}

// highlight a string
void highlight(Editor* editor, char* str, size_t length) {
    int outLen = length;
    size_t i = 0, tmp;
    if (!editor->currSyntax) {
        displayDraw(&editor->a, "%s", str);
        return;
    }
    while (i < length) {
        bool ifStrSyntax = isSyntax(editor, &outLen, &str[i], i);
        if (ifStrSyntax) {
            tmp = i;
            while (i < tmp + outLen) {
                processBG(&editor->a, editor->config.colours[BACKGROUND_COLOUR]);
                processFG(&editor->a, editor->currMatches[editor->currMatchPos].colour);
                displayDraw(&editor->a, "%c", str[i]);
                i++;

                while (editor->currMatchPos > 0 && i >= (size_t)editor->currMatches[editor->currMatchPos].ends.rm_eo) {
                    editor->currMatchPos--;
                }

                isSyntax(editor, &outLen, &str[i], i);
            }
            free(editor->currMatches);
            editor->currMatches = NULL;
            editor->currMatchPos = 0;
            editor->matchLen = 0;
        }
        else {
            reset(&editor->a);
            processBG(&editor->a, editor->config.colours[BACKGROUND_COLOUR]);
            displayDraw(&editor->a, "%c", str[i]);
            i++;
        }
    }
}

// show linenumbers on the screen
void lineNumShow(Editor* editor) {
    // disable default line wrapping
    displayDraw(&editor->a, "\033[?7l");
    int showLine = 1 + editor->c.scrollY;
    int yPos = 1;
    
    while (yPos < editor->width - 1) {
        int showStrLen = snprintf(0, 0, "%d", showLine);

        if (showLine <= editor->linenum) {
            Row* row = &editor->row[showLine - 1];
            if (editor->config.linenums) {
                // draw the line number to the screen
                // make sure the linenumber is coloured correctly
                if (editor->c.cursorY == (size_t)showLine)
                    processFG(&editor->a, editor->config.colours[LINENUM_TEXT_ACTIVE_COLOUR]);
                else 
                    processFG(&editor->a, editor->config.colours[LINENUM_TEXT_COLOUR]);
    
                pos(0, yPos, &editor->a);
                processBG(&editor->a, editor->config.colours[LINENUM_COLOUR]);
                for (size_t i = 0; i < editor->config.disLine - (showStrLen) - 2; i++) displayDraw(&editor->a, " ");
                displayDraw(&editor->a, "%d ", showLine);
            }
            else {
                pos(0, yPos, &editor->a);
            }

            // append the text in
            reset(&editor->a);
            size_t scrollX = 0;
            if (editor->c.cursorY == (size_t)showLine) scrollX = editor->c.scrollX;
            size_t spliceLen = row->tlen - scrollX;
            if (spliceLen >= (size_t)editor->height - 6) spliceLen = editor->height - 6;
            char* toStr = utftomb(&row->tab[scrollX], spliceLen, &spliceLen);
            
            highlight(editor, toStr, spliceLen);
            free(toStr);
        }
        else {
            pos(0, yPos, &editor->a);
        }
        // coloring in the background
        processBG(&editor->a, editor->config.colours[BACKGROUND_COLOUR]);
        displayDraw(&editor->a, "\033[K");

        reset(&editor->a);
        showLine++;
        yPos++;
    }
}

// buffer 
void bufferDisplay(Editor* editor) {
    lineNumShow(editor);
    getWindowSize(editor);   
    drawStatusBar(editor);
    systemShowMessage(editor);
}

void initConfig(Editor* editor) {
    editor->config.tabcount = DEFAULT_TABCOUNT;
    editor->config.disLine = DEFAULT_DISLINE;
    editor->config.linenums = true;

    editor->syntaxes = NULL;
    editor->syntaxLen = 0;
    editor->currSyntax = editor->syntaxes;

    // initialize velte colours, by default all colours will be transparent
    editor->config.colours = check_malloc(COLOUR_CONFIG_LENGTH * sizeof(Rgb));
    for (int i = 0; i < COLOUR_CONFIG_LENGTH; i++) {
        editor->config.colours[i].isTransparent = true;
    }

    // read the config file
    if (!readConfigFile(editor, ".velte")) {
        seteditorMsg(editor, "Could not load config file");
        createSyntax(editor, " ", "No Syntax\0");
    }
    if (checkExtension(editor, editor->filename) == 0) {
        createSyntax(editor, " ", "No Syntax\0");
        editor->currSyntax = editor->syntaxes + editor->syntaxLen - 1;
    }
}

void initCursor(Cursor* cursor) {
    cursor->cursorX = 0;
    cursor->cursorY = 1;
    cursor->scrollX = 0;
    cursor->scrollY = 0;
    cursor->tabX = 0;
    cursor->utfJump = 0;
}

// init
void showDisplay(Editor* editor) {
    editor->modified = 0;
    editor->tabRem = 0;
    editor->a.string = NULL;
    editor->a.length = 0;
    editor->currMatches = NULL;
    editor->matchLen = 0;
    editor->currMatchPos = 0;
    seteditorMsg(editor, "");
    initCursor(&editor->c);
    initConfig(editor);

    while (1) {
        bufferDisplay(editor);
        positionCursor(editor->c, editor->config.disLine, &editor->a);
        writeDisplay(&editor->a);
        controlKeypresses(editor);
        scroll(editor, &editor->c, editor->row[editor->c.cursorY - 1].str, editor->config.disLine);
    }
}
