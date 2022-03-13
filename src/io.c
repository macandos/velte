#include "io.h"
#include "display.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

DisplayInit openFile(char* filename, DisplayInit dinit) {
    FILE* file;
    file = fopen(filename, "r");
    if (!file) return dinit; // don't do anything else

    char* line = NULL;
    dinit.row = NULL;
    size_t len = 0;
    ssize_t read;
    int linenums = 0;

    while ((read = getline(&line, &len, file)) != -1) {
        // allocate space in struct array
        dinit.row = realloc(dinit.row, sizeof(DisplayInit) * (linenums + 1));
        dinit.row[linenums].line = malloc(read + 1);

        memcpy(dinit.row[linenums].line, line, read);
        dinit.row[linenums].length = read;
        dinit.row[linenums].line[read] = '\0'; 

        linenums++;
    }

    dinit.linenum = linenums;
    fclose(file);
    free(line);
    return dinit;
}
