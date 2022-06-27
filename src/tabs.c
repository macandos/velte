#include "tabs.h"
#include "display.h"
#include "keypresses.h"

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

// check if the current char in a line is a tab
int isTab(DisplayInit* dinit, int pos, int y) {
    Row* row = &dinit->row[y];
    if (row->line[pos] == '\t') return 0;
    return 1;
}

// calculate the total number of tabs in a select line
int getTabCount(DisplayInit* dinit, int y) {
    Row* row = &dinit->row[y];
    int tabcount = 0;
    for (int i = 0; i < row->length; i++) {
        if (isTab(dinit, i, y) == 0) {
            tabcount++;
        }
    }
    return tabcount;
}

void tabChange(DisplayInit* dinit, int pos) {
    Row* row = &dinit->row[pos];
    free(row->tabs.tab);
    int j = 0;
    int tabcount = getTabCount(dinit, pos);

    // malloc space for the tab space, tablength is 4
    row->tabs.tab = (char*)malloc(row->length + (tabcount * 3) + 1);
    for (int i = 0; i < row->length; i++) {
        // loop through everything; and append a certain amount of spaces
        // if the current char is a tab
        if (isTab(dinit, i, pos) == 0) {
            do {
                row->tabs.tab[j] = ' ';
                j++;
            }
            while (j % 4 != 0);
            continue;
        }
        row->tabs.tab[j] = row->line[i];
        j++;
    }
    row->tabs.tab[j] = '\0';
    row->tabs.tlen = j;
}

void controlOffsetX(DisplayInit* dinit) {
    int tabOffset = dinit->d.cursorX - dinit->d.offsetX;
    Row* row = &dinit->row[dinit->d.cursorY - 1];
    dinit->d.offsetX = 0;
    int temp = 6;
    for (int i = 0; i < row->length; i++) {
        if (i == tabOffset - 5) break;
        if (isTab(dinit, i, dinit->d.cursorY - 1) == 0) {
            temp++;
            while ((temp - 6) % 4 != 0) {
                if (i == tabOffset - 6) break;
                temp++;
                dinit->d.offsetX++;
            }
        }
        else temp++;
    }
    dinit->d.cursorX = temp - 1;
}

// control cursor movement through tabs
void cursorMovementTab(DisplayInit* dinit, char c) {
    Row* row = &dinit->row[dinit->d.cursorY - 1];
    switch (c) {
        case ARROW_LEFT: {
            dinit->d.calculateLengthStop++;
            if (isTab(dinit, dinit->d.tabX - 7, dinit->d.cursorY - 1) == 1) {
                dinit->d.cursorX--;
                return;
            }
            break;
        }
        case ARROW_RIGHT:
            dinit->d.calculateLengthStop--;
            dinit->d.cursorX++;
            if (isTab(dinit, dinit->d.tabX - 6, dinit->d.cursorY - 1) == 1) {
                return;
            }
            break;
    }
    /*
        we use the calculateLengthStop variable to sum up all the length before the cursor
        the variable moves accordingly to when the user inputs ARROW_LEFT, or ARROW_RIGHT.
    */
    dinit->d.offsetX = 0;
    dinit->d.cursorX = 6;
    for (int i = 0; i < row->length - dinit->d.calculateLengthStop; i++) {
        if (isTab(dinit, i, dinit->d.cursorY - 1) == 0) {
            dinit->d.cursorX++;
            while ((dinit->d.cursorX - 6) % 4 != 0) {
                dinit->d.cursorX++;
                dinit->d.offsetX++;
            }
        }
        else dinit->d.cursorX++;
    }
}