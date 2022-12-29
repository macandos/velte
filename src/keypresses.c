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

// position the X cursor to the correct position
void positionCursor(Editor* editor, App* a) {
    size_t x, y;
    if (!editor->m.isStatus) {
        x = editor->c.maskX + 1;
        y = editor->width - 1;
    }
    else {
        x = editor->c.cursorX + editor->config.disLine;
        y = editor->c.cursorY;
    }
    pos(
        x - editor->c.scrollX + editor->c.utfJump,
        y,
        a
    );
}

// calculate the real position of the Y cursor
int calcCursorY(Editor* editor)  {
    return editor->c.cursorY + editor->c.scrollY - 1;
}

// append a character into the line buffer
void appChar(Editor* editor, size_t pos, uint32_t character) {
    Row* row = &editor->row[editor->c.posY];

    row->str = check_realloc(row->str, (row->length + 1) * sizeof(uint32_t));
    memmove(&row->str[pos + 1], &row->str[pos], (row->length - pos - 1) * sizeof(uint32_t));
    row->str[row->length] = '\0';
    row->length++;
    row->str[pos] = character;
    editor->c.cursorX++;

}

// delete a charater from the selected line buffer
void delChar(Editor* editor, size_t pos) {
    Row* row = &editor->row[editor->c.posY];
    memmove(&row->str[pos], &row->str[pos + 1], (row->length - pos - 1) * sizeof(uint32_t));
    editor->c.cursorX--;
    row->length--;
}

// check if the current char in a line is a tab
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

// check if cursor goes ahead to the next tab and moves it back
void checkAheadTab(Editor* editor, size_t xBeforeNL, size_t xAfterNL, int rF) {
    editor->c.posY = calcCursorY(editor);
    if (isTab(editor, editor->c.tabX - 1, editor->c.posY) == 0 && (xBeforeNL % editor->config.tabcount) != 0 
            && (xAfterNL % editor->config.tabcount) != 0) {
        editor->c.cursorX -= rF + 1;
        editor->c.tabX--;
    }
    editor->tabRem = xBeforeNL;
}

int countTabX(Editor* editor, size_t end, bool countX) {
    Row* row = &editor->row[calcCursorY(editor)];
    editor->c.tabX = 0;
    editor->c.cursorX = 0;
    editor->c.utfJump = 0;
    int roundTo = 0;
    for (size_t i = 0; (countX == true ? editor->c.cursorX < end : i < end); i++) {
        editor->c.tabX++;
        editor->c.cursorX++;
        if (isTab(editor, i, editor->c.cursorY - 1) == 0) {
            roundTo = roundXToTabSpace(editor->config.tabcount, editor->c.cursorX);
            editor->c.cursorX += roundTo;
        }

        calculateCharacterWidth(editor, row->str[i]);
    }
    return roundTo;
}

void tabChange(Editor* editor, int pos) {
    Row* row = &editor->row[pos];
    free(row->tabs.tab);
    size_t j = 0;
    int tabcount = getTabCount(editor, pos);

    // malloc space for the tab space, tablength is 4 by default
    row->tabs.tab = check_malloc((row->length + (tabcount * (editor->config.tabcount - 1)) + 1) * sizeof(uint32_t));
    for (size_t i = 0; i < row->length; i++) {
        // loop through everything; and append a certain amount of spaces
        // if the current char is a tab
        if (isTab(editor, i, pos) == 0) {
            do {
                row->tabs.tab[j] = ' ';
                j++;
            }
            while (j % editor->config.tabcount != 0);
            continue;
        }
        row->tabs.tab[j] = row->str[i];
        j++;
    }
    row->tabs.tab[j] = '\0';
    row->tabs.tlen = j;
}

int roundXToTabSpace(unsigned char tabcount, int num) {
    return ((num + tabcount - 1) & ~(tabcount - 1)) - num;
}

// check if X is too low, and then bring the cursor up to the correct position
void xUnderSix(Editor* editor) {
    if (editor->c.cursorX + editor->config.disLine < 6) {
        if (editor->c.scrollX > 0) return;
        if (editor->c.cursorY == 1) {
            editor->c.cursorX++;
            editor->c.tabX++;
            return;
        }
        checkCursorLines(editor, ARROW_UP);
    }
}

// check if X is over the line, then moves it to the next one
void overLine(Editor* editor) {
    Row* row = &editor->row[editor->c.posY];
    if (editor->c.posY >= editor->linenum - 1) return;
    if (editor->c.cursorX > row->tabs.tlen - 1) {
        checkCursorLines(editor, ARROW_DOWN);
        editor->c.cursorX = 0;
        editor->c.tabX = 0;
    }
}

// check the cursor going up and down lines
void checkCursorLines(Editor* editor, char c) {
    Row* row = &editor->row[editor->c.posY];

    if (editor->c.cursorX == row->tabs.tlen - 1) {
        c == ARROW_UP ? editor->c.cursorY-- : editor->c.cursorY++;
        editor->c.posY = calcCursorY(editor);
        row = &editor->row[editor->c.posY];
        editor->c.cursorX = row->tabs.tlen - 1;
        editor->c.tabX = row->length - 1;
        return;
    }
    if (isTab(editor, editor->c.tabX, editor->c.posY) == 0) {
        editor->c.cursorX = editor->tabRem;
    }
    c == ARROW_UP ? editor->c.cursorY-- : editor->c.cursorY++;
    editor->c.posY = calcCursorY(editor);

    row = &editor->row[editor->c.posY];
    if (editor->c.cursorX > row->tabs.tlen - 1) {
        editor->c.cursorX = row->tabs.tlen - 1;
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
    free(editor->row[pos].tabs.tab);
    memmove(&editor->row[pos], &editor->row[pos + 1], sizeof(Row) * (editor->linenum - pos - 1));
    editor->linenum--;
}

// handle whenever the backspace key is pressed
void deleteChar(Editor* editor) {
    if (editor->c.cursorX > 0) {
        delChar(editor, editor->c.tabX - 1);
        tabChange(editor, editor->c.posY);
        editor->c.tabX--;
    }
    else {
        Row* row = &editor->row[editor->c.posY];
        if (editor->c.cursorY == 1) return;
 
        editor->c.cursorY--;
        editor->c.posY--;
        editor->c.cursorX = editor->row[editor->c.posY].tabs.tlen - 1;
        editor->c.tabX = editor->row[editor->c.posY].length - 1;

        appendLine(&editor->row[editor->c.posY], row->str, row->length);
        removeLine(editor->c.posY + 1, editor);
        tabChange(editor, editor->c.posY);
    }
}

// handle whenever the enter key is pressed
void handleEnter(Editor* editor) {
    Row* row = &editor->row[editor->c.posY];

    // create a new line, append the text to it, 
    // and make the length of the old line shorter
    int length = (row->length - editor->c.tabX);
    appendRow(&row->str[editor->c.tabX], length, editor->c.posY, editor);

    row = &editor->row[editor->c.posY];
    row->length = editor->c.tabX + 1;
    row->str[row->length - 1] = '\0';
    editor->c.cursorY++;
}

// handle scrolling of the cursor
void scroll(Editor* editor) {
    if (editor->c.cursorY > editor->width - 2) {
        editor->c.scrollY += editor->c.cursorY - editor->width + 2;
        editor->c.cursorY = editor->width - 2;
    }
    else if (editor->c.cursorY < 3 && editor->c.scrollY > 0) {
        editor->c.scrollY--;
        editor->c.cursorY = 3;
    }
    editor->c.scrollX = horizontalScrollOffset(NULL, 
                                            editor->c.cursorX + editor->c.utfJump, 
                                            editor->height - editor->config.disLine, 
                                            editor->row[calcCursorY(editor)].tabs.tlen);

}

void processKeypresses(uint32_t character, Editor* editor) {
    editor->c.posY = calcCursorY(editor);
    Row* row = &editor->row[editor->c.posY];

    // switch between all possible keypresses
    if (character != '\0') {
        switch (character) {
            case CTRL_KEY('x'):
                printf("\033c");
                exit(0);
                break;
            case CTRL_KEY('s'):
                writeFile(editor);
                editor->modified = 0;
                return;
            case CTRL_KEY('o'):
                editor->c.cursorX = 0;
                break;
            case CTRL_KEY('p'):
                editor->c.cursorX = row->tabs.tlen;
                break;
            // Enter
            case '\n':
            case '\r':
                handleEnter(editor);
                tabChange(editor, editor->c.posY);
                editor->c.cursorX = 0;
                editor->c.tabX = 0;
                editor->modified = 1;
                break;
            // Backspacing
            case BACKSPACE_KEY:
                deleteChar(editor);
                editor->modified = 1;
                break;
            // arrow keys
            case ARROW_DOWN:
            case ARROW_UP: {
                if (editor->c.posY == 0 && character == ARROW_UP) {
                    break;
                }
                if (editor->c.posY >= editor->linenum - 1 && character == ARROW_DOWN) {
                    break;
                }
                size_t xBeforeNL = editor->c.cursorX;
                checkCursorLines(editor, character);
                size_t xAfterNL = editor->c.cursorX;
                int rF = countTabX(editor, editor->c.cursorX, true);
                checkAheadTab(editor, xBeforeNL, xAfterNL, rF);
                return;
            }
            case ARROW_RIGHT:
                if (editor->c.cursorX >= row->tabs.tlen - 1 && editor->c.posY >= editor->linenum - 1) {
                    editor->c.cursorX = row->tabs.tlen - 1;
                    break;
                }
                editor->c.tabX++;
                editor->c.cursorX++;
                overLine(editor);
                break;
            case ARROW_LEFT:
                editor->c.cursorX--;
                editor->c.tabX--;
                xUnderSix(editor);
                break;
            // if it does not match with any keypresses, we assume that its a character
            // to be inputted into the line buffer
            default:
                appChar(editor, editor->c.tabX, character);
                tabChange(editor, editor->c.posY);
                editor->c.tabX++;
                editor->modified = 1;
                break;
        }
        countTabX(editor, editor->c.tabX, false);
        editor->tabRem = editor->c.cursorX;
    }
}