#include "io.h"
#include "display.h"
#include "keypresses.h"
#include "error.h"
#include "uchar.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

// split a file into an array of individual lines
char** splitFileIntoLines(char* filename, int* linenums) {
    off_t idx = 0;
    off_t fileLength = getFileLength(filename);
    int linenum = 0;
    char** ret = NULL;
    
    // open the file and store it in one string
    int fd = open(filename, O_RDONLY);
    char* fileStr = check_malloc(fileLength + 1);
    char* line = NULL;
    read(fd, fileStr, fileLength);
    fileStr[fileLength] = '\0';
    close(fd);

    // loop through the file string and split it into the individual lines
    // and make sure unwanted characters such as `\n` and `\r` are not included
    while (1) {
        int linelen = 0;
        while (fileStr[idx] != '\n' && fileStr[idx] != '\r') {
            idx++;
            linelen++;
            if (idx >= fileLength) break;
        }
        if (idx > fileLength) break;
        idx++;

        line = check_malloc(linelen + 1);
        memcpy(line, &fileStr[idx - linelen - 1], linelen);
        line[linelen] = '\0';

        // append the line into the buffer
        ret = check_realloc(ret, sizeof(char*) * (linenum + 1));
        ret[linenum] = check_malloc(linelen + 1);
        memcpy(ret[linenum], line, linelen);
        ret[linenum][linelen] = '\0';
        linenum++;
    }
    free(fileStr);
    free(line);
    *linenums = linenum;
    return ret;
}

void openFile(char* filename, Editor* editor) {
    editor->linenum = 0;
    uint32_t space = SPACE_KEY;
    editor->filename = filename;
    editor->row = NULL;
    off_t fileLength = getFileLength(filename);

    // if the file contains nothing, or it doesn't exist (when
    // `getFileLength` returns -1, append an empty line
    if (fileLength <= 0) {
        createRow(editor->linenum, editor);
        changeStr(&space, 1, editor->linenum, editor);
        return;
    }

    int linenums;
    char** lines = splitFileIntoLines(filename, &linenums);
    size_t realLen;

    for (int i = 0; i < linenums; i++) {
        createRow(i, editor);
        uint32_t* uIntStr = mbtoutf(lines[i], strlen(lines[i]), &realLen);
        changeStr(uIntStr, realLen + 1, i, editor);
    }
}

// reads a velte config file and parses it
int readConfigFile(Editor* editor, char* filename) {
    off_t fileLength = getFileLength(filename);
    if (fileLength < 0) {
        seteditorMsg(editor, "No such file");
        return 0;
    }
    int linenums;
    char** lines = splitFileIntoLines(filename, &linenums);
    for (int i = 0; i < linenums; i++) {
        editorCommand(editor, lines[i]);
    }
    return 1;
}

// append a string onto a line, replacing the old string in the array.
void changeStr(uint32_t* str, size_t length, int pos, Editor* editor) {
    editor->row[pos].str = check_malloc((length + 1) * sizeof(uint32_t));
    memcpy(editor->row[pos].str, str, (length) * sizeof(uint32_t));
    editor->row[pos].length = length;

    editor->row[pos].tab = NULL;
    editor->row[pos].tlen = 0;
    tabChange(editor, pos);

    editor->linenum++;
}

// get file length
off_t getFileLength(const char* filename) {
    struct stat len;
    if (stat(filename, &len) == 0) {
        return len.st_size;
    }
    return -1;
}

// create a new row with string as text
void createRow(int pos, Editor* editor) {
    editor->row = check_realloc(editor->row, sizeof(Row) * (editor->linenum + 1));
    memmove(&editor->row[pos + 1], &editor->row[pos], sizeof(Row) * (editor->linenum - pos));
}

void appendRow(uint32_t* str, size_t length, int pos, Editor* editor) {
    createRow(pos, editor);
    changeStr(str, length, pos + 1, editor);
}

// save file to disk
void writeFile(Editor* editor) {
    // append all the rows to a string
    int length = 0;
    
    // get the total size of the file
    for (int i = 0; i < editor->linenum; i++)
        length += editor->row[i].length;

    uint32_t* totalStr = check_malloc((length + 1) * sizeof(uint32_t));
    int strLen = 0;

    // append all the lines into one string
    for (int i = 0; i < editor->linenum; i++) {
        memcpy(&totalStr[strLen], editor->row[i].str, editor->row[i].length * sizeof(uint32_t));

        strLen += editor->row[i].length;
        totalStr[strLen - 1] = '\n';
    }
    size_t rL;
    char* toWrite = utftomb(totalStr, length, &rL);
    char* filename = editor->filename;
    
    // create a new file if it doesnt exist
    if (!filename) {
        filename = editorPrompt(editor, "Save as: \0");
        if (filename == 0) {
            seteditorMsg(editor, "");
            return;
        }
        seteditorMsg(editor, "Program saved!");
    }

    // open the file and append all the contents of the total file string to it
    int fd = open(filename, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        char errorStr[19 + strlen(filename)];
        snprintf(errorStr, sizeof(errorStr), "Cannot save file: %s", strerror(errno));
        seteditorMsg(editor, errorStr);
        free(totalStr);
        close(fd);
        return;
    }

    ftruncate(fd, rL);
    write(fd, toWrite, rL);
    close(fd);
    free(totalStr);
    editor->filename = filename;
    checkExtension(editor, editor->filename);
}