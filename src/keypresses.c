#include "keypresses.h"
#include "display.h"
#include "uchar.h"
#include "error.h"
#include "io.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

static void (*commandArr[])(Editor*, char**, int) = {
    &gotoX,
    &gotoY,
    &config,
    &display,
    &syntax,
    &fileend,
    &load,
};

static const char* colourConfigList[] = {
    "linenumtextcolour",
    "linenumtextactivecolour",
    "linenumcolour",
    "statustextcolour",
    "statuscolour",
    "backgroundcolour",
    "messagecolour",
};

static const char* commandList[] = {
    "gotox",
    "gotoy",
    "config",
    "display",
    "syntax",
    "fileend",
    "load",
};

// parse an inputted terminal command
void editorCommand(Editor* editor, char* command) {
    char* tok = NULL;
    const char* delim = " ";
    tok = strtok(command, delim);
    if (!tok) return;
    int arrayLength = (sizeof(commandList) / sizeof(char*));

    // check if the input was a comment
    if (strcmp(tok, "--") == 0) {
        return;
    }
    
    // check if the text has a command in it, and save the index so we can call the
    // correct function afterwards
    bool isCommand = false;
    int i;
    for (i = 0; i < arrayLength; i++) {
        if (strcmp(tok, commandList[i]) == 0) {
            isCommand = true;
            break;
        }
    }
    if (!isCommand) {
        char errMsg[21 + strlen(tok)];
        if (snprintf(errMsg, sizeof(errMsg), "Unknown command: '%s'", tok))
            seteditorMsg(editor, errMsg);
        return;
    }

    char** arguments = NULL;
    size_t arglen = 0;

    //store arguments in a list
    while (1) {
        tok = strtok(NULL, delim);
        if (!tok) break;
        arguments = check_realloc(arguments, sizeof(char*) * (arglen + 1));

        arguments[arglen] = check_malloc(strlen(tok) + 1);
        memcpy(arguments[arglen], tok, strlen(tok));
        arguments[arglen][strlen(tok)] = '\0';
        arglen++;
    }

    // execute the respected function
    (*commandArr[i])(editor, arguments, arglen);
    free(arguments);
}

// define editor commands
// syntax usage: gotox <x>
// changes the x position to the argument
void gotoX(Editor* editor, char** args, int arglen) {
    if (args == NULL || arglen != 1) {
        seteditorMsg(editor, "gotox: Invalid arguments");
        return;
    }
    size_t by;
    int result;

    // if the user put end, go to the end of the line
    if (strcmp(args[0], "end") == 0) {
        by = editor->row[editor->c.cursorY - 1].tlen;
        result = 1;
    }
    else {
        // convert the string into an integer
        result = editorConvStringToSizeT(editor, &by, args[0], 10);
    }
    if (by > editor->row[editor->c.cursorY - 1].tlen || !result) return;
    editor->c.cursorX = by;
    countTabX(editor, editor->c.cursorX, false);
}

// editor usage: gotoy <y>
// changes the y position to the argument
void gotoY(Editor* editor, char** args, int arglen) {
    if (args == NULL || arglen != 1) {
        seteditorMsg(editor, "gotoy: Invalid arguments");
        return;
    }
    size_t by;
    int result;

    // go to the end of the file if the user has inputted end
    if (strcmp(args[0], "end") == 0) {
        by = editor->linenum;
        result = 1;
    }
    else {
        // convert the string into an integer
        result = editorConvStringToSizeT(editor, &by, args[0], 10);
    }
    if (!result || by > (size_t)editor->linenum) return;
    editor->c.cursorY = by;

    // reposition x if needed
    if (editor->c.cursorX > editor->row[editor->c.cursorY - 1].length) {
        editor->c.cursorX = editor->row[editor->c.cursorY - 1].length;
        countTabX(editor, editor->c.cursorX, false);
    }

}

// editor usage: config <config> (arguments)
// change velte's config
void config(Editor* editor, char** args, int arglen) {
    if (args == NULL || arglen != 2) {
        seteditorMsg(editor, "config: Invalid arguments");
        return;
    }
    // changing the distance between the edge of the screen and the linenums
    else if (strcmp(args[0], "disline") == 0) {
        size_t by;
        int result = editorConvStringToSizeT(editor, &by, args[1], 10);
        if (by > (size_t)editor->height || !result) {
            seteditorMsg(editor, "config: disline: Invalid length");
            return;
        }
        editor->config.disLine = by;
    }
    // changing the tabcount
    else if (strcmp(args[0], "tabcount") == 0) {
        size_t by;
        int result = editorConvStringToSizeT(editor, &by, args[1], 10);
        if (result) {
            editor->config.tabcount = by;
            tabChange(editor, editor->c.cursorY - 1);
        }
        else {
            seteditorMsg(editor, "config: Invalid tabcount");
            return;
        }
    }
    // changing if the linenumbers should be displayed or not
    else if (strcmp(args[0], "linenums") == 0) {
        if (strcmp(args[1], "true") == 0) {
            editor->config.linenums = true;
            editor->config.disLine = DEFAULT_DISLINE;
        }
        else if (strcmp(args[1], "false") == 0) {
            editor->config.linenums = false;
            editor->config.disLine = 1;
        }
        else {
            seteditorMsg(editor, "config: Invalid option");
        }
    }
    else {
        seteditorMsg(editor, "config: Invalid setting");
    }
}

// syntax usage: display <displayConfig> <colourcode>
// deals with changing how the look of velte looks
void display(Editor* editor, char** args, int arglen) {
    if (args == NULL || arglen != 2) {
        seteditorMsg(editor, "display: Invalid arguments");
        return;
    }
    Rgb rgbVal;

    // check if the argument entered is correct
    int arrayLength = COLOUR_CONFIG_LENGTH;
    bool isConfig = false;
    int i;
    for (i = 0; i < arrayLength; i++) {
        if (strcmp(args[0], colourConfigList[i]) == 0) {
            isConfig = true;
            break;
        }
    }
    if (!isConfig) {
        seteditorMsg(editor, "display: Unknown colour config");
        return;
    }

    // check if the colour code should be transparent; instead of the usual colour code
    if (strcmp(args[1], "transparent") == 0) {
        editor->config.colours[i].isTransparent = true;
    }
    else {
        int check = checkRgb(editor, &rgbVal, args[1]);
        if (!check) {
            seteditorMsg(editor, "display: Invalid colour");
            return;
        }
        editor->config.colours[i].isTransparent = false;
        editor->config.colours[i] = rgbVal;
    }
}

// syntax usage: syntax <colourcode> <regex> (curr)
// deals with syntax highlighting customization
void syntax(Editor* editor, char** args, int arglen) {
    if (args == NULL || (arglen != 2 && arglen != 3)) {
        seteditorMsg(editor, "syntax: Invalid arguments");
        return;
    }
    Rgb rgbVal;
    bool sub = false;
    int check = checkRgb(editor, &rgbVal, args[0]);
    if (!check) {
        seteditorMsg(editor, "syntax: Invalid colour");
        return;
    }
    // if the 'curr' keyword is present, then we will append a syntax rule to
    // what currSyntax points to, instead of the last syntax struct in the list
    if (arglen == 3) {
        if (strcmp(args[2], "curr") == 0) {
            editor->currSyntax->map = appendSyntax(editor, true, args[1], rgbVal, sub);
            return;
        }
        // should the syntax rule support colouring in any submatches?
        else if (strcmp(args[2], "sub") == 0) {
            sub = true;
        }
        else {
            seteditorMsg(editor, "syntax: Invalid arguments");
            return;
        }
    }
    editor->syntaxes[editor->syntaxLen - 1].map = appendSyntax(editor, false, args[1], rgbVal, sub);
}

// syntax usage: fileend <regex> (syntaxname)
// links any file endings with any syntax highlighting files
void fileend(Editor* editor, char** args, int arglen) {
    if (args == NULL || arglen != 2) {
        seteditorMsg(editor, "fileend: Invalid arguments");
        return;
    }
    createSyntax(editor, args[0], args[1]);
}

// usage: load <file>
// loads a config file so velte can parse it
void load(Editor* editor, char** args, int arglen) {
    if (args == NULL || arglen > 1) {
        seteditorMsg(editor, "load: Invalid arguments");
        return;
    }
    readConfigFile(editor, args[0]);
}

// position the cursor to the correct position
void positionCursor(Cursor cursor, int disLine, App* a) {
    pos(
        cursor.cursorX - cursor.scrollX + cursor.utfJump + disLine,
        cursor.cursorY - cursor.scrollY,
        a
    );
}

// append a character into the line buffer
void appChar(Editor* editor, size_t pos, uint32_t character) {
    Row* row = &editor->row[editor->c.cursorY - 1];

    row->str = check_realloc(row->str, (row->length + 2) * sizeof(uint32_t));
    memmove(&row->str[pos + 1], &row->str[pos], (row->length - pos - 1) * sizeof(uint32_t));
    row->str[row->length] = '\0';
    row->str[pos] = character;
    editor->c.cursorX++;
    row->length++;

}

// delete a charater from the selected line buffer
void delChar(Editor* editor, size_t pos) {
    Row* row = &editor->row[editor->c.cursorY - 1];
    memmove(&row->str[pos], &row->str[pos + 1], (row->length - pos - 1) * sizeof(uint32_t));
    editor->c.cursorX--;
    row->length--;
}

// check if the current char in a line is a tab ('\t')
int isTab(Editor* editor, size_t pos, int y) {
    Row* row = &editor->row[y];
    if (row->str[pos] == TAB_KEY) return 0;
    return 1;
}

// calculate the total number of tabs in a select line
int getTabCount(Editor* editor, int y) {
    Row* row = &editor->row[y];
    int tabcount = 0;
    for (size_t i = 0; i < row->length; i++) {
        if (isTab(editor, i, y) == 0) {
            tabcount++;
        }
    }
    return tabcount;
}

// if the cursor is in the middle of a tab, move it back to the start
void checkAheadTab(Editor* editor, size_t xBeforeNL, size_t xAfterNL, int rF) {
    if (isTab(editor, editor->c.tabX - 1, editor->c.cursorY - 1) == 0 && (xBeforeNL % editor->config.tabcount) != 0 
            && (xAfterNL % editor->config.tabcount) != 0) {
        editor->c.cursorX -= rF + 1;
        editor->c.tabX--;
    }
    editor->tabRem = xBeforeNL;
}

// loop through the buffer and position the cursor so that it fits in with the tabs
int countTabX(Editor* editor, size_t end, bool countX) {
    Row* row = &editor->row[editor->c.cursorY - 1];
    editor->c.tabX = 0;
    editor->c.cursorX = 0;
    editor->c.utfJump = 0;
    int roundTo = 0;

    for (size_t i = 0; (countX == true ? editor->c.cursorX < end : i < end); i++) {
        editor->c.tabX++;
        editor->c.cursorX++;
        if (isTab(editor, i, editor->c.cursorY - 1) == 0) {
            // rounding x to the nearest multiple of tabcount
            roundTo = roundXToTabSpace(editor->config.tabcount, editor->c.cursorX);
            editor->c.cursorX += roundTo;
        }

        // making sure the x cursor fits in with multi width characters
        if (i >= (size_t)editor->c.scrollX)
            calculateCharacterWidth(editor, row->str[i], &editor->c.utfJump);
    }
    return roundTo;
}

void tabChange(Editor* editor, int pos) {
    Row* row = &editor->row[pos];
    free(row->tab);
    size_t j = 0;
    int tabcount = getTabCount(editor, pos);

    // malloc space for the tab space, tablength is 4 by default
    row->tab = check_malloc((row->length + (tabcount * (editor->config.tabcount - 1)) + 1) * sizeof(uint32_t));
    for (size_t i = 0; i < row->length; i++) {
        // loop through everything; and append a certain amount of spaces
        // if the current char is a tab
        if (isTab(editor, i, pos) == 0) {
            do {
                row->tab[j] = ' ';
                j++;
            }
            while (j % editor->config.tabcount != 0);
        }
        else {
            row->tab[j] = row->str[i];
            j++;
        }
    }
    row->tab[j] = '\0';
    row->tlen = j;
}

int roundXToTabSpace(size_t tabcount, int num) {
    if (num == 0) return tabcount;
    return (num + tabcount - 1 - (num + tabcount - 1) % tabcount) - num;
}

// check if X is too low, and then bring the cursor up to the correct position
void xUnderDisLine(Editor* editor) {
    if (editor->c.cursorY == 1) return;
    
    editor->c.cursorY--;
    Row* row = &editor->row[editor->c.cursorY - 1];
    editor->c.cursorX = row->tlen - 1;
    editor->c.tabX = row->length - 1;
}

// check if X is over the line, then moves it to the next one
void overLine(Editor* editor) {
    Row* row = &editor->row[editor->c.cursorY - 1];
    if (editor->c.cursorY - 1 >= (size_t)editor->linenum - 1) return;
    if (editor->c.cursorX > row->tlen - 1) {
        checkCursorLines(editor, ARROW_DOWN);
        editor->c.cursorX = 0;
        editor->c.tabX = 0;
    }
}

// check the cursor going up and down lines
void checkCursorLines(Editor* editor, char c) {
    Row* row = &editor->row[editor->c.cursorY - 1];

    // if the cursor is at the end of a line and is moving to a another 
    // line, make sure it always goes to the end of the next/prev line
    if (editor->c.cursorX == row->tlen - 1) {
        c == ARROW_UP ? editor->c.cursorY-- : editor->c.cursorY++;
        row = &editor->row[editor->c.cursorY - 1];
        editor->c.cursorX = row->tlen - 1;
        editor->c.tabX = row->length - 1;
        return;
    }
    // make sure the cursor stays inline when moving up and down with tabs
    if (isTab(editor, editor->c.tabX, editor->c.cursorY - 1) == 0) {
        editor->c.cursorX = editor->tabRem;
    }
    c == ARROW_UP ? editor->c.cursorY-- : editor->c.cursorY++;
    c == ARROW_UP ? row-- : row++;
    // make sure that the cursor never goes past the end of the next/prev line, when it is
    // not at the end
    if (editor->c.cursorX > row->tlen - 1) {
        editor->c.cursorX = row->tlen - 1;
        editor->c.tabX = row->length - 1;
    }
}

// append a line to the buffer array
void appendLine(Row* row, uint32_t* s, size_t len) {
    row->str = check_realloc(row->str, sizeof(uint32_t) * (row->length + len + 1));
    memcpy(&row->str[row->length - 1], s, len * sizeof(uint32_t));
    
    row->length += len - 1;
    row->str[row->length] = '\0';
}

// remove a line, and free all of its components
void removeLine(int pos, Editor* editor) {
    free(editor->row[pos].str);
    free(editor->row[pos].tab);
    memmove(&editor->row[pos], &editor->row[pos + 1], sizeof(Row) * (editor->linenum - pos - 1));
    editor->linenum--;
}

// handle whenever the backspace key is pressed
void deleteChar(Editor* editor) {
    if (editor->c.cursorX > 0) {
        delChar(editor, editor->c.tabX - 1);
        tabChange(editor, editor->c.cursorY - 1);
        editor->c.tabX--;
    }
    else {
        Row* row = &editor->row[editor->c.cursorY - 1];
        if (editor->c.cursorY == 1) return;
 
        editor->c.cursorY--;
        editor->c.cursorX = editor->row[editor->c.cursorY - 1].tlen - 1;
        editor->c.tabX = editor->row[editor->c.cursorY - 1].length - 1;

        // append the line to the previous line, and remove the current line
        appendLine(&editor->row[editor->c.cursorY - 1], row->str, row->length);
        removeLine(editor->c.cursorY, editor);
        tabChange(editor, editor->c.cursorY - 1);
    }
}

// handle whenever the enter key is pressed
void handleEnter(Editor* editor) {
    Row* row = &editor->row[editor->c.cursorY - 1];

    // create a new line, append the text to it, 
    // and make the length of the old line shorter
    int length = (row->length - editor->c.tabX);
    appendRow(&row->str[editor->c.tabX], length, editor->c.cursorY - 1, editor);

    row = &editor->row[editor->c.cursorY - 1];
    row->length = editor->c.tabX + 1;
    row->str[row->length - 1] = '\0';
}

// handle scrolling of the cursor
void scroll(Editor* editor, Cursor* cursor, uint32_t* str, size_t disLine) {
    // increment scrollY whenever the cursor goes past the end of the screen
    if (cursor->cursorY > (size_t)editor->width + cursor->scrollY - 2) {
        cursor->scrollY = cursor->cursorY - editor->width + 2;
    }
    // when the cursor scrolls up, make sure scrollY is going down
    else if (cursor->cursorY < (size_t)cursor->scrollY + 1 && cursor->scrollY > 0) {
        cursor->scrollY = cursor->cursorY - 1;
    }
    // we will do the same for horizontal scrolling
    // the '- 5' would be the offset as we dont want it to go directly next to the end before scrolling
    if (cursor->cursorX + cursor->utfJump + disLine > (size_t)editor->height + cursor->scrollX - 5) {
        cursor->scrollX = cursor->cursorX + cursor->utfJump + disLine + 5 - editor->height;
    }
    else if (cursor->cursorX + cursor->utfJump < (size_t)cursor->scrollX && cursor->scrollX > 0) {
        cursor->scrollX = cursor->cursorX + cursor->utfJump;
    }
    // make sure even after changing scrollX does cursorX remain inline with multi-space characters
    cursor->utfJump = 0;
    for (size_t i = cursor->scrollX; i < cursor->tabX; i++) {
        calculateCharacterWidth(editor, str[i], &cursor->utfJump);
    }
}

void processKeypresses(uint32_t character, Editor* editor) {
    Row* row = &editor->row[editor->c.cursorY - 1];
    if (character != '\0') {
        switch (character) {
            case ESCAPE_KEY: {
                    seteditorMsg(editor, "");
                    char* prompt = editorPrompt(editor, "Enter command: \0");
                    if (prompt != 0) editorCommand(editor, prompt);
                }
                break;
            case CTRL_KEY('x'):
                // clear the screen and exit
                clearDisplay(&editor->a);
                exit(0);
                break;
            case CTRL_KEY('s'):
                writeFile(editor);
                editor->modified = 0;
                return;
            // Enter
            case '\n':
            case '\r':
                handleEnter(editor);
                tabChange(editor, editor->c.cursorY - 1);
                editor->c.cursorX = 0;
                editor->c.tabX = 0;
                editor->modified = 1;
                editor->c.cursorY++;
                break;
            // Backspacing
            case BACKSPACE_KEY:
                deleteChar(editor);
                editor->modified = 1;
                break;
            // arrow keys
            case ARROW_DOWN:
            case ARROW_UP: {
                // making sure the cursor doesnt go before line 1
                if (editor->c.cursorY - 1 == 0 && character == ARROW_UP) {
                    break;
                }
                // and making sure it doesnt go past the total amount of lines
                if (editor->c.cursorY - 1 >= (size_t)editor->linenum - 1 && character == ARROW_DOWN) {
                    break;
                }
                size_t xBeforeNL = editor->c.cursorX;
                checkCursorLines(editor, character);
                size_t xAfterNL = editor->c.cursorX;
                int rF = countTabX(editor, editor->c.cursorX, true);
                checkAheadTab(editor, xBeforeNL, xAfterNL, rF);

                editor->c.scrollX = editor->c.cursorX + editor->c.utfJump + editor->config.disLine + 5 - editor->height;
                if (editor->c.scrollX < 0) editor->c.scrollX = 0;
                scroll(editor, &editor->c, editor->row[editor->c.cursorY - 1].str, editor->config.disLine);
                bufferDisplay(editor);
                return;
            }
            case ARROW_RIGHT:
                // keeping the cursor in boundaries with the line
                if (editor->c.cursorX >= row->tlen - 1 && editor->c.cursorY - 1 >= (size_t)editor->linenum - 1) {
                    editor->c.cursorX = row->tlen - 1;
                    break;
                }
                editor->c.tabX++;
                editor->c.cursorX++;
                overLine(editor);
                break;
            case ARROW_LEFT:
                if (editor->c.tabX < 1 && editor->c.scrollX == 0) {
                    xUnderDisLine(editor);
                }
                else {
                    editor->c.cursorX--;
                    editor->c.tabX--;
                }
                break;
            // if it does not match with any keypresses, we assume that its a character
            // to be inputted into the line buffer
            default:
                appChar(editor, editor->c.tabX, character);
                tabChange(editor, editor->c.cursorY - 1);
                editor->c.tabX++;
                editor->modified = 1;
                break;
        }
        countTabX(editor, editor->c.tabX, false);
        editor->tabRem = editor->c.cursorX;
        scroll(editor, &editor->c, editor->row[editor->c.cursorY - 1].str, editor->config.disLine);
    }
}