#include "keypresses.h"
#include "display.h"
#include "error.h"
#include "io.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// so we can process CTRL + (SOME KEY)
#define CTRL_KEY(k) ((k) & 0x1f)

// append a character into our program
void appChar(DisplayInit* dinit, int pos, char character) {
    Row* row = &dinit->row[dinit->d.cursorY - 1];
    dinit->d.cursorX++;
        
    row->line = realloc(row->line, row->length + 2);
    memmove(&row->line[pos + 1], &row->line[pos], row->length - (pos + 1));

    row->length++;
    row->line[pos] = character;
    row->line[row->length] = '\0';
}

// delete a charater from our program
void delChar(DisplayInit* dinit, int pos) {
    Row* row = &dinit->row[dinit->d.cursorY - 1];

    if (pos < 0 || pos > row->length) return;
    memmove(&row->line[pos], &row->line[pos + 1], row->length - (pos));
    dinit->d.cursorX--;
    row->length--;
}

// check the cursor
void checkCursor(DisplayInit* dinit, char c) {
    Row* row = &dinit->row[dinit->d.cursorY - 1];

    if (dinit->d.cursorX < 6) {
        if (dinit->d.cursorY > 1) {
            dinit->d.cursorY--;

            row = &dinit->row[dinit->d.cursorY - 1];
            dinit->d.cursorX = row->length + 5;
        }
        else
            dinit->d.cursorX = 6;
    }   
    else if (dinit->d.cursorX > row->length + 5) {
        if (dinit->d.cursorY >= dinit->linenum) return;

        if (c == ARROW_RIGHT) {
            dinit->d.cursorY++;
            dinit->d.cursorX = 6;
        }
        else {
            c == ARROW_UP ? dinit->d.cursorY-- : dinit->d.cursorY++;
            if (c == ARROW_UP) row = &dinit->row[dinit->d.cursorY];
            else row = &dinit->row[dinit->d.cursorY - 2];

            dinit->d.cursorX = row->length + 5;
            c == ARROW_UP ? dinit->d.cursorY++ : dinit->d.cursorY--;
        }
    }
    else if (c < 3 && dinit->d.cursorX == row->length + 5) {
        c == ARROW_UP ? dinit->d.cursorY-- : dinit->d.cursorY++;
        row = &dinit->row[dinit->d.cursorY - 1];
        dinit->d.cursorX = row->length + 5;
    }
}

// append a line to the terminal
void appendLine(Row* row, char* s, size_t len) {
    row->line = realloc(row->line, row->length + len + 1);
    memcpy(&row->line[row->length], s, len);

    //if (row->line[row->length] == '\0') row->length--;
    row->length += len;
}

// remove a line, and free all of its components
void removeLine(int pos, DisplayInit* dinit) {
    if (pos < 1 || pos >= dinit->linenum) return;
    free(dinit->row[pos].line);
    memmove(&dinit->row[pos], &dinit->row[pos + 1], sizeof(Row) * (dinit->linenum - pos - 1));
    dinit->linenum--;
}

// handle whenever the backspace key is pressed
void deleteChar(DisplayInit* dinit) {
    Row* row = &dinit->row[dinit->d.cursorY - 1];

    if (dinit->d.cursorX > 6) {
        delChar(dinit, dinit->d.cursorX - 7);
    }
    else {
        dinit->d.cursorX = dinit->row[dinit->d.cursorY - 2].length + 6;
        appendLine(&dinit->row[dinit->d.cursorY - 2], row->line, row->length);
        removeLine(dinit->d.cursorY - 1, dinit);
        dinit->d.cursorY--;
    }
}

// handle whenever the enter key is pressed
void handleEnter(DisplayInit* dinit) {
    Row* row = &dinit->row[dinit->d.cursorY - 1];
 
    if (dinit->d.cursorX != 6) {
        int length = row->length - dinit->d.cursorX + 6;
        createRow(&row->line[dinit->d.cursorX - 6], length, dinit->d.cursorY, dinit);

        dinit->row[dinit->d.cursorY - 1].length = dinit->d.cursorX - 6;
        dinit->d.cursorY++;
    }
    else {
        createRow(" ", 1, dinit->d.cursorY, dinit);
        dinit->d.cursorY++;
    }
}

void processKeypresses(char character, DisplayInit *dinit) {
    Row* row = &dinit->row[dinit->d.cursorY - 1];

    // switch between all possible keypresses
    if (character != '\0') {
        switch (character) {
            case CTRL_KEY('x'):
                //exit
                errorHandle("exit");
                break;
            case CTRL_KEY('u'):
                writeFile(dinit);
                if (dinit->modified == 1) 
                    strncpy(dinit->msg, "Program saved successfully!\0", 28);
                dinit->modified = 0;
                break;
            // Enter
            case '\n':
                handleEnter(dinit);
                
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
                if (dinit->d.cursorY > dinit->linenum) break;
            case ARROW_UP:
                if (dinit->d.cursorX < row->length + 5) {
                    if (character == ARROW_UP) {
                        if (dinit->d.cursorY == 1) break;
                        dinit->d.cursorY--;
                    }
                    else {
                        dinit->d.cursorY++;
                    }
                    if (dinit->row[dinit->d.cursorY - 1].length + 5 == dinit->d.cursorX) {
                        break;
                    }
                }            
                checkCursor(dinit, character);
                break;
            case ARROW_RIGHT:
                dinit->d.cursorX++;
                checkCursor(dinit, ARROW_RIGHT);
                break;
            case ARROW_LEFT:
                dinit->d.cursorX--;
                checkCursor(dinit, ARROW_LEFT);
                break;
            default:
                appChar(dinit, dinit->d.cursorX - 6, character);
                dinit->modified = 1;
                break;
        }
    }
}