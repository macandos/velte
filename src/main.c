#include <stdio.h>
#include <string.h>
#include <langinfo.h>
#include <stdlib.h>
#include <locale.h>

#include "raw.h"
#include "error.h"
#include "display.h"
#include "io.h"
#include "uchar.h"
#include "keypresses.h"

void init(char* filename) {
    Editor editor;

    raw();
    if (strcmp(nl_langinfo(CODESET), "UTF-8") == 0) {
        editor.config.isUtf8 = true;
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