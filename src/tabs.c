#include "tabs.h"
#include "display.h"
#include "keypresses.h"

#include <stdlib.h>
#include <ctype.h>

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
    row->tabs.tab = malloc(row->length + (tabcount * 3) + 1);
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
    Row* row = &dinit->row[dinit->d.cursorY - 1];
    dinit->d.offsetX = 0;
    int temp = 6;
    for (int i = 0; i < row->length; i++) {
        if (temp == dinit->d.cursorX) break;
        if (isTab(dinit, i, dinit->d.cursorY - 1) == 0) {
            do {
                temp++;
                dinit->d.offsetX++;
            }
            while ((temp - 6) % 4 != 0);
            dinit->d.offsetX--;
            continue;
        }
        temp++;
    }
    dinit->d.cursorX = temp - 1;
}

// control user movement through tabs
void cursorMovementTab(DisplayInit* dinit, char c) {
    Row* row = &dinit->row[dinit->d.cursorY - 1];
    if (isTab(dinit, dinit->d.tabX - 7, dinit->d.cursorY - 1) == 1) {
        c == ARROW_LEFT ? dinit->d.cursorX-- : dinit->d.cursorX++;
        return;
    }

    // switch (c) {
    //     case ARROW_LEFT:
    //         do {
    //             dinit->d.cursorX--;
    //             dinit->d.offsetX--;
    //             if (dinit->d.cursorX <= 6) break;
    //             if (isgraph(row->line[dinit->d.cursorX - 7])) break;
    //         }
    //         while ((dinit->d.cursorX - 6) % 4 != 0);
    //         dinit->d.offsetX++;
    //         break;
    //     case ARROW_RIGHT:
    //         for (int i = 0; i < 4; i++) {
    //             dinit->d.cursorX++;
    //             dinit->d.offsetX++;
    //             if ((dinit->d.cursorX - 6) % 4 == 0) break;
    //         }
    //         dinit->d.offsetX--;
    //         break;
    // }
}