#include "keypresses.h"
#include "display.h"
#include "error.h"
#include "tabs.h"
#include "io.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// so we can process CTRL + (SOME KEY)
#define CTRL_KEY(k) ((k) & 0x1f)

// append a character into our program
void appChar(DisplayInit* dinit, int pos, char character) {
    Row* row = &dinit->row[dinit->d.cursorY - 1];    
    row->line = realloc(row->line, row->length + 2);
    memmove(&row->line[pos + 1], &row->line[pos], (row->length - pos));

    row->length++;
    row->line[pos] = character;
    dinit->d.cursorX++;
}

// delete a charater from our program
void delChar(DisplayInit* dinit, int pos) {
    Row* row = &dinit->row[dinit->d.cursorY - 1];
    memmove(&row->line[pos], &row->line[pos + 1], row->length - pos);
    dinit->d.cursorX--;
    row->length--;
}

// check the cursor
void checkCursor(DisplayInit* dinit, char c) {
    Row* row = &dinit->row[dinit->d.cursorY - 1];
    if (dinit->d.cursorX < 6) {
        if (dinit->d.cursorY == 1) {
            dinit->d.cursorX++;
            return;
        }
        dinit->d.cursorY--;
        row = &dinit->row[dinit->d.cursorY - 1];
        dinit->d.cursorX = row->tabs.tlen + 5;
    }
    else if (c < 3 && dinit->d.cursorX == row->tabs.tlen + 5) {
        c == ARROW_UP ? dinit->d.cursorY-- : dinit->d.cursorY++;
        row = &dinit->row[dinit->d.cursorY - 1];
        dinit->d.cursorX = row->tabs.tlen + 5;
    }
    else if (dinit->d.cursorX > row->tabs.tlen + 5) {
        if (c == ARROW_RIGHT) {
            dinit->d.cursorY++;
            if (dinit->d.cursorY > dinit->linenum) {
                dinit->d.cursorY--;
                return;
            }
            else
                dinit->d.cursorX = 6;
        }
        else {
            c == ARROW_UP ? dinit->d.cursorY-- : dinit->d.cursorY++;
            if (c == ARROW_UP) row = &dinit->row[dinit->d.cursorY];
            else row = &dinit->row[dinit->d.cursorY - 2];

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
    Row* row = &dinit->row[dinit->d.cursorY - 1];

    if (dinit->d.cursorX > 6) {
        if (isTab(dinit, dinit->d.tabX - 7, dinit->d.cursorY - 1) == 0) {
            for (int i = 0; i < 3; i++) {
                dinit->d.tabX = dinit->d.cursorX - dinit->d.offsetX;
                if (isTab(dinit, dinit->d.cursorX - 6, dinit->d.cursorY - 1) == 0) break;
                dinit->d.cursorX--;
                dinit->d.offsetX--;
            }
            delChar(dinit, dinit->d.tabX - 7);
        }
        else delChar(dinit, dinit->d.tabX - 7);
    }
    else {
        int y = dinit->d.cursorY - 1;
        if (y == 0) return;

        dinit->d.cursorX = dinit->row[y - 1].tabs.tlen + 5;
        appendLine(&dinit->row[y - 1], row->line, row->length);
        removeLine(y, dinit);
        dinit->d.cursorY--;
    }
}

// handle whenever the enter key is pressed
void handleEnter(DisplayInit* dinit) {
    int y = dinit->d.cursorY - 1;
    Row* row = &dinit->row[y]; 
    
    int length = row->length - dinit->d.tabX + 6;
    createRow(&row->line[dinit->d.tabX - 6], length, y + 1, dinit);
    row = &dinit->row[y];
    row->length = dinit->d.tabX - 6;

    // append a null-terminator at the end of the string
    row->line = realloc(row->line, row->length + 1);
    row->line[row->length] = '\0';
    row->length++;
    dinit->d.cursorY++;
    dinit->d.offsetX = 0;
}

void processKeypresses(char character, DisplayInit *dinit, App* a) {
    Row* row = &dinit->row[dinit->d.cursorY - 1];

    // switch between all possible keypresses
    dinit->d.tabX = dinit->d.cursorX - dinit->d.offsetX;
    if (character != '\0') {
        switch (character) {
            case CTRL_KEY('x'):
                //exit
                errorHandle("exit");
                break;
            case CTRL_KEY('u'):
                writeFile(dinit, a);
                if (dinit->modified == 1) 
                    strncpy(dinit->msg, "Program saved successfully!\0", 28);
                dinit->modified = 0;
                break;
            case CTRL_KEY('o'):
                dinit->d.cursorX = 6;
                dinit->d.offsetX = 0;
                break;
            case CTRL_KEY('p'):
                dinit->d.cursorX = row->tabs.tlen + 5;
                break;
            case '\t':
                appChar(dinit, dinit->d.tabX - 6, character);
                tabChange(dinit, dinit->d.cursorY - 1);
                controlOffsetX(dinit);
                break;
            // Enter
            case '\n':
            case '\r':
                handleEnter(dinit);
                tabChange(dinit, dinit->d.cursorY - 2);
                dinit->d.cursorX = 6;
                dinit->modified = 1;
                break;
            // Backspacing
            case 127:
                deleteChar(dinit);
                tabChange(dinit, dinit->d.cursorY - 1);
                dinit->modified = 1;
                break;
            // arrow keys
            case ARROW_DOWN:
                if (dinit->d.cursorY >= dinit->linenum) break;
            case ARROW_UP:
                if (dinit->d.cursorX < row->tabs.tlen + 5) {
                    if (character == ARROW_UP) {
                        if (dinit->d.cursorY == 1) break;
                        dinit->d.cursorY--;
                    }
                    else {
                        dinit->d.cursorY++;
                    }

                    if (dinit->row[dinit->d.cursorY - 1].tabs.tlen + 5 
                    == dinit->d.cursorX) {
                        break;
                    }
                }
                checkCursor(dinit, character);
                break;
            case ARROW_RIGHT:
                cursorMovementTab(dinit, character);
                dinit->d.cursorX++;
                if (dinit->d.cursorX > row->tabs.tlen + 5 && dinit->d.cursorY == dinit->linenum)
                    dinit->d.cursorX = row->tabs.tlen + 5; 
                checkCursor(dinit, ARROW_RIGHT);
                break;
            case ARROW_LEFT:
                cursorMovementTab(dinit, character);
                checkCursor(dinit, ARROW_LEFT);
                break;
            default:
                appChar(dinit, dinit->d.tabX - 6, character);
                tabChange(dinit, dinit->d.cursorY - 1);
                dinit->modified = 1;
                break;
        }
    }
}