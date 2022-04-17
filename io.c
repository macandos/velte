#include "io.h"
#include "display.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

// create new row with string as text
void createRow(char* text, int length, int pos, DisplayInit* dinit) {
    // allocate space in the array
    dinit->row = realloc(dinit->row, sizeof(DisplayInit) * (dinit->linenum + 1));
    memmove(&dinit->row[pos], &dinit->row[pos - 1], sizeof(Row) * (dinit->linenum - pos));
    dinit->row[pos].line = malloc(length + 1);

    memcpy(dinit->row[pos].line, text, length);
    dinit->row[pos].length = length;
    dinit->row[pos].line[length] = '\0'; 

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
    if (!file) return; // don't do anything else

    dinit->linenum = 0;
    char* line = NULL;
    dinit->row = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, file)) != -1) {
        while (handleUnwantedChars(line, read) == 0) line[read - 1] = ' ';
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
    int length = 0;
    
    // get total size
    for (int i = 0; i < dinit->linenum; i++)
        length += dinit->row[i].length;

    char* totalStr = malloc(length);
    char* buff = totalStr;
    for (int i = 0; i < dinit->linenum; i++) {
        memcpy(buff, dinit->row[i].line, dinit->row[i].length);

        buff[dinit->row[i].length - 1] = '\0';
        buff += dinit->row[i].length - 1;
        *buff = '\n';
        buff++;
    }

    // open the file and append all contents
    // into it.
    int fd = open(dinit->filename, O_RDWR | O_CREAT, 0644);
    ftruncate(fd, length);
    write(fd, totalStr, length);
    close(fd);
    free(totalStr);
}
