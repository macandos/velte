#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "raw.h"
#include "error.h"
#include "display.h"
#include "io.h"

void init(char* filename) {
    DisplayInit dinit;

    // Enter Raw mode
    raw();

    // Open file
    dinit = openFile(filename, dinit);
    
    // Show the display
    showDisplay(dinit); 
}

int main(int argc, char* argv[]) {
    init(argv[1]);
}