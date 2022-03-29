#include "keypresses.h"
#include "display.h"
#include "error.h"
#include "io.h"

#include <stdlib.h>
#include <string.h>

// so we can process CTRL + (SOME KEY)
#define CTRL_KEY(k) ((k) & 0x1f)

// append a character into our program
void appChar(DisplayInit* dinit, int pos, char character) {
    Row* row = &dinit->row[dinit->d.cursorY - 1];

    if (character != '\0') {
        dinit->d.cursorX++;
        if (pos < 0 || pos > row->length) pos = row->length;
        row->line = realloc(row->line, row->length + 2);
        memmove(&row->line[pos + 1], &row->line[pos], row->length - (pos + 1));

        row->length++;
        row->line[pos] = character;
    }
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
    else if (dinit->d.cursorY < 1) {
        dinit->d.cursorY++;
    }
    else if (dinit->d.cursorY > dinit->linenum) {
        dinit->d.cursorY--;
    }   
    else if (c < 3 && dinit->d.cursorX > row->length + 5) {
        c == ARROW_UP ? dinit->d.cursorY-- : dinit->d.cursorY++;
        if (c == ARROW_UP) row = &dinit->row[dinit->d.cursorY];
        else row = &dinit->row[dinit->d.cursorY - 2];

        dinit->d.cursorX = row->length + 5;
        c == ARROW_UP ? dinit->d.cursorY++ : dinit->d.cursorY--;
    }
    else if (c < 3 && dinit->d.cursorX == row->length + 5) {
        c == ARROW_UP ? dinit->d.cursorY-- : dinit->d.cursorY++;
        row = &dinit->row[dinit->d.cursorY - 1];
        dinit->d.cursorX = row->length + 5;
    }
}

DisplayInit processKeypresses(char character, DisplayInit dinit) {
    Row* row = &dinit.row[dinit.d.cursorY - 1];

    // switch between all possible keypresses
    if (character != '\0') {
        switch (character) {
            case CTRL_KEY('x'):
                //exit
                errorHandle("exit");
                break;
            case CTRL_KEY('u'):
                writeFile(&dinit);
                dinit.modified = 0;
                break;
            // Enter
            case '\n':
                dinit.d.cursorX = 0;
                createRow("\n", 1, dinit.d.cursorY, &dinit);

                dinit.d.cursorX = 6;
                dinit.d.cursorY++;
                break;
            // Backspacing
            case 127:
                delChar(&dinit, dinit.d.cursorX - 7);
                dinit.modified = 1;
                break;
            // arrow keys
            case ARROW_UP:
            case ARROW_DOWN:
                if (dinit.d.cursorX < row->length + 5) {
                    character == ARROW_UP ? dinit.d.cursorY-- : dinit.d.cursorY++;
                }
                    
                checkCursor(&dinit, character);
                break;
            case ARROW_RIGHT:
                if (dinit.d.cursorX < row->length + 5)
                    dinit.d.cursorX++;
                checkCursor(&dinit, ARROW_RIGHT);
                break;
            case ARROW_LEFT:
                dinit.d.cursorX--;
                checkCursor(&dinit, ARROW_LEFT);
                break;
            default:
                // TODO: FIX THIS FUNCTION!!
                appChar(&dinit, dinit.d.cursorX - 6, character);
                dinit.modified = 1;
                break;
        }
    }

    return dinit;
}