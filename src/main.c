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

Editor editor;

void deinit() {
    endRaw();

    for (int i = 0; i < editor.syntaxLen; i++) {
        regfree(&editor.syntaxes->fileEndings);
    }
    free(editor.syntaxes);
    free(editor.row);
}

void init(char* filename) {
    raw();
    atexit(deinit);
    setlocale(LC_CTYPE, "");
    if (strncmp(nl_langinfo(CODESET), "UTF-8", 5) == 0) {
        editor.config.isUtf8 = true;
    }
    getWindowSize(&editor);
    openFile(filename, &editor);
    showDisplay(&editor);
}

int main(int argc, char* argv[]) {
    init(argv[1]);
}