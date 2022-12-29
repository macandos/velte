#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <langinfo.h>
#include <stdlib.h>
#include <locale.h>
#include <stdint.h>

#include "raw.h"
#include "error.h"
#include "display.h"
#include "io.h"
#include "keypresses.h"
#include "uchar.h"


void init(char* filename) {
    Editor editor;

    raw();
    if (strcmp(nl_langinfo(CODESET), "UTF-8") == 0) {
        editor.config.isUtf8 = 1;
    }
    setlocale(LC_CTYPE, "");
    getWindowSize(&editor);

    initConfig(&editor);
    openFile(filename, &editor);
    showDisplay(&editor);
}

int main(int argc, char* argv[]) {
    init(argv[1]);
}