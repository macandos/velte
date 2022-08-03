#include "io.h"
#include "display.h"
#include "tabs.h"
#include "keypresses.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

/* append a string onto a line, replacing the old
string in the array. */
void changeStr(char* str, size_t length, int pos, DisplayInit* dinit) {
    if (pos > dinit->linenum) return;

    // allocate and append into the array
    dinit->row[pos].line = malloc(length);
    memcpy(dinit->row[pos].line, str, length);
    dinit->row[pos].length = length;
}

// create new row with string as text
void createRow(char* text, size_t length, int pos, DisplayInit* dinit) {
    if (pos > dinit->linenum) return;

    // allocate space in the array
    dinit->row = realloc(dinit->row, sizeof(Row) * (dinit->linenum + 1));
    memmove(&dinit->row[pos + 1], &dinit->row[pos], sizeof(Row) * (dinit->linenum - pos));
    dinit->row[pos].line = malloc(length + 1);

    memcpy(dinit->row[pos].line, text, length);
    dinit->row[pos].line[length] = '\0';
    dinit->row[pos].length = length;

    dinit->row[pos].tabs.tab = NULL;
    dinit->row[pos].tabs.tlen = 0;
    tabChange(dinit, pos);
    dinit->linenum++;
}

/* check if string has an unwanted character at the end
e.g \n or \r */
int handleUnwantedChars(const char* text, ssize_t length) {
    if (length > 0) {
        if (text[length - 1] == '\r' || text[length - 1] == '\n') {
            return 0;
        }
    }

    return 1; /* fail */
}

// open the file
void openFile(char* filename, DisplayInit *dinit) {
    FILE* file;
    file = fopen(filename, "r");

    dinit->linenum = 0;
    char* line = NULL;
    dinit->row = NULL;

    /* do something if the file does not exist;
    e.g. create a new one */
    if (!file) {
        createRow(" " , 1, dinit->linenum, dinit);
        dinit->filename = NULL;
        return;
    }

    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, file)) != -1) {
        while (handleUnwantedChars(line, read) == 0) line[read - 1] = ' ';
        createRow(line, read, dinit->linenum, dinit);
    }
    dinit->filename = filename;

    // free stuff
    fclose(file);
    free(line);
}

// save file to disk
void writeFile(DisplayInit* dinit) {
    if (!dinit->filename) {
        dinit->filename = systemScanfUser(dinit, "Save as: \0");
        if (dinit->filename == 0) {
            setDinitMsg(dinit, "", 0);
            return;
        }
        setDinitMsg(dinit, "Program saved!", 14);
    }

    // append all the rows to a string
    int length = 0;
    
    // get total size
    for (int i = 0; i < dinit->linenum; i++)
        length += dinit->row[i].length;

    char* totalStr = malloc(length);
    int strlen = 0;
    for (int i = 0; i < dinit->linenum; i++) {
        memcpy(&totalStr[strlen], dinit->row[i].line, dinit->row[i].length);

        strlen += dinit->row[i].length - 1;
        totalStr[strlen] = '\n';
        strlen++;
    }

    // open the file and append all contents
    // into it.
    int fd = open(dinit->filename, O_RDWR | O_CREAT, 0644);
    ftruncate(fd, length);
    write(fd, totalStr, length);
    close(fd);
    free(totalStr);
} 
