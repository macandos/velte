#include "keypresses.h"
#include "display.h"
#include "error.h"
#include "tabs.h"
#include "io.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

// append a character into our program
void appChar(DisplayInit* dinit, int pos, char character) {
    int scrollOffset = dinit->d.cursorY + dinit->d.scrollY - 1;
    Row* row = &dinit->row[scrollOffset];    
    row->line = realloc(row->line, row->length + 2);
    memmove(&row->line[pos + 1], &row->line[pos], (row->length - pos));

    row->length++;
    row->line[pos] = character;
    dinit->d.cursorX++;
}

// delete a charater from our program
void delChar(DisplayInit* dinit, int pos) {
    int scrollOffset = dinit->d.cursorY + dinit->d.scrollY - 1;
    Row* row = &dinit->row[scrollOffset];
    memmove(&row->line[pos], &row->line[pos + 1], row->length - pos);
    dinit->d.cursorX--;
    row->length--;
}

// check the cursor
void checkCursor(DisplayInit* dinit, char c) {
    int scrollOffset = dinit->d.cursorY + dinit->d.scrollY - 1;
    Row* row = &dinit->row[scrollOffset];
    if (dinit->d.cursorX < 6) {
        if (dinit->d.scrollX > 0) return;
        if (dinit->d.cursorY == 1) {
            dinit->d.cursorX++;
            dinit->d.calculateLengthStop--;
            return;
        }
        dinit->d.cursorY--;
        row = &dinit->row[scrollOffset - 1];
        dinit->d.cursorX = row->tabs.tlen + 5;
    }
    else if (c < 3 && dinit->d.cursorX == row->tabs.tlen + 5) {
        if (scrollOffset >= dinit->linenum - 1 && c == ARROW_DOWN) return;
        c == ARROW_UP ? dinit->d.cursorY-- : dinit->d.cursorY++;
        scrollOffset = dinit->d.cursorY + dinit->d.scrollY - 1;
        row = &dinit->row[scrollOffset];
        dinit->d.cursorX = row->tabs.tlen + 5 + dinit->d.offsetX;
    }
    else if (dinit->d.cursorX > row->tabs.tlen + 5) {
        if (dinit->d.cursorY == dinit->linenum) return;
        if (c == ARROW_RIGHT) {
            dinit->d.cursorY++;
            dinit->d.cursorX = 6;
        }
        else {
            c == ARROW_UP ? dinit->d.cursorY-- : dinit->d.cursorY++;
            scrollOffset = dinit->d.cursorY + dinit->d.scrollY - 1;

            if (c == ARROW_UP) row = &dinit->row[scrollOffset + 1];
            else row = &dinit->row[scrollOffset - 1];

            dinit->d.cursorX = row->tabs.tlen + 5;
            c == ARROW_UP ? dinit->d.cursorY++ : dinit->d.cursorY--;
        }
    }
}

// append a line to the terminal
void appendLine(Row* row, char* s, size_t len) {
    row->line = realloc(row->line, row->length + len + 1);
    if (row->line[row->length] == '\0' || 
    row->line[row->length - 1] == '\0') row->length--;
    memcpy(&row->line[row->length], s, len);
    row->length += len;
}

// remove a line, and free all of its components
void removeLine(int pos, DisplayInit* dinit) {
    free(dinit->row[pos].line);
    free(dinit->row[pos].tabs.tab);
    memmove(&dinit->row[pos], &dinit->row[pos + 1], sizeof(Row) * (dinit->linenum - pos - 1));
    dinit->linenum--;
}

// handle whenever the backspace key is pressed
void deleteChar(DisplayInit* dinit) {
    int scrollOffset = dinit->d.cursorY + dinit->d.scrollY - 1;
    Row* row = &dinit->row[scrollOffset];

    if (dinit->d.cursorX > 6) {
        if (isTab(dinit, dinit->d.tabX - 7, scrollOffset) == 0) {
            delChar(dinit, dinit->d.tabX - 7);
            tabChange(dinit, scrollOffset);
            controlOffsetX(dinit);
        }
        else delChar(dinit, dinit->d.tabX - 7);
        tabChange(dinit, scrollOffset);
    }
    else {
        if (dinit->d.cursorY == 1) return;
 
        dinit->d.cursorY--;
        scrollOffset = dinit->d.cursorY + dinit->d.scrollY - 1;
        dinit->d.cursorX = dinit->row[scrollOffset].tabs.tlen + 5;
        controlOffsetX(dinit);

        appendLine(&dinit->row[scrollOffset], row->line, row->length);
        removeLine(scrollOffset + 1, dinit);
        tabChange(dinit, scrollOffset);
    }
}

// handle whenever the enter key is pressed
void handleEnter(DisplayInit* dinit) {
    int scrollOffset = dinit->d.cursorY + dinit->d.scrollY - 1;
    Row* row = &dinit->row[scrollOffset]; 
    
    int length = row->length - dinit->d.tabX + 6;
    createRow(&row->line[dinit->d.tabX - 6], length, scrollOffset + 1, dinit);
    row = &dinit->row[scrollOffset];
    row->length = dinit->d.tabX - 6;

    // append a null-terminator at the end of the string
    row->line = realloc(row->line, row->length + 1);
    row->line[row->length] = '\0';
    row->length++;
    dinit->d.cursorY++;
    dinit->d.offsetX = 0;
}

// handle scrolling
void scroll(DisplayInit* dinit) {
    if (dinit->d.cursorY > dinit->width - 2) {
        dinit->d.scrollY++;
        dinit->d.cursorY = dinit->width - 2;
    }
    else if (dinit->d.cursorY < 3 && dinit->d.scrollY > 0) {
        dinit->d.scrollY--;
        dinit->d.cursorY = 3;
    }
}

void processKeypresses(char character, DisplayInit *dinit) {
    int scrollOffset = dinit->d.cursorY + dinit->d.scrollY - 1;
    Row* row = &dinit->row[scrollOffset];

    // switch between all possible keypresses
    dinit->d.tabX = dinit->d.cursorX - dinit->d.offsetX;
    if (character != '\0') {
        switch (character) {
            case CTRL_KEY('x'):
                //exit
                errorHandle("exit");
                break;
            case CTRL_KEY('u'):
                writeFile(dinit);
                dinit->modified = 0;
                break;
            case CTRL_KEY('o'):
                dinit->d.cursorX = 6;
                dinit->d.offsetX = 0;
                break;
            case CTRL_KEY('p'):
                dinit->d.cursorX = row->tabs.tlen + 5;
                controlOffsetX(dinit);
                break;
            // Enter
            case '\n':
            case '\r':
                handleEnter(dinit);
                tabChange(dinit, scrollOffset);
                dinit->d.cursorX = 6;
                dinit->modified = 1;
                break;
            // Backspacing
            case 127:
                deleteChar(dinit);
                dinit->modified = 1;
                break;
            // arrow keys
            case ARROW_DOWN:
                if (scrollOffset >= dinit->linenum) break;
            case ARROW_UP:
                if (dinit->d.cursorY == 1 && character == ARROW_UP) {
                    break;
                }
                if (dinit->d.cursorX < row->tabs.tlen + 5) {
                    if (character == ARROW_UP) dinit->d.cursorY--;
                    else dinit->d.cursorY++;

                    scrollOffset = dinit->d.cursorY + dinit->d.scrollY - 1;
                    if (dinit->row[scrollOffset].tabs.tlen + 5 == dinit->d.cursorX) 
                        break;
                }
                checkCursor(dinit, character);
                controlOffsetX(dinit);
                break;
            case ARROW_RIGHT:
                cursorMovementTab(dinit, character);
                if (dinit->d.cursorX > row->tabs.tlen + 5 && dinit->d.cursorY == dinit->linenum) {
                    dinit->d.cursorX = row->tabs.tlen + 5; 
                    dinit->d.calculateLengthStop = 1;
                }
                checkCursor(dinit, ARROW_RIGHT);
                break;
            case ARROW_LEFT:
                cursorMovementTab(dinit, character);
                checkCursor(dinit, ARROW_LEFT);
                break;
            default:
                appChar(dinit, dinit->d.tabX - 6, character);
                tabChange(dinit, scrollOffset);
                controlOffsetX(dinit);
                dinit->modified = 1;
                break;
        }
        // scroll
        scroll(dinit);
    }
}