#include "io.h"
#include "display.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// create new row with string as text
void createRow(char* text, int length, int pos, DisplayInit* dinit) {
    // allocate space in the array
    dinit->row = realloc(dinit->row, sizeof(DisplayInit) * (dinit->linenum + 1));
    memmove(&dinit->row[pos + 1], &dinit->row[pos], sizeof(Row) * (dinit->linenum - pos));
    dinit->row[pos].line = malloc(length + 1);

    memcpy(dinit->row[pos].line, text, length);
    dinit->row[pos].length = length;
    dinit->row[pos].line[length] = '\0'; 

    dinit->linenum++;
}

// open the file
void openFile(char* filename, DisplayInit *dinit) {
    FILE* file;
    file = fopen(filename, "r");
    if (!file) return; // don't do anything else

    dinit->linenum = 0;
    char* line = NULL;
    dinit->row = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, file)) != -1) {
        createRow(line, read, dinit->linenum, dinit);
    }
    
    dinit->row[dinit->linenum - 1].length++;
    dinit->filename = filename;
    
    // free stuff
    fclose(file);
    free(line);
}

// save file to disk
void writeFile(DisplayInit* dinit) {
    // append all the rows to a string
    char* totalStr = NULL;
    int length = 0;

    // get total size
    for (int i = 0; i < dinit->linenum; i++) 
        length += dinit->row[i].length + 1;

    totalStr = malloc(length);
    for (int i = 0; i < dinit->linenum; i++) {
        strncat(totalStr, dinit->row[i].line, dinit->row[i].length + 1);
    }

    // open the file and append all contents
    // into it.
    FILE* file = fopen(dinit->filename, "w+");
    fwrite(totalStr, strlen(totalStr), 1, file);
}
