#ifndef KEYPRESSES_H_
#define KEYPRESSES_H_

#include "structs.h"
#include <stdbool.h>

#define TAB_KEY 0X09
#define SPACE_KEY 0x20
#define BACKSPACE_KEY 0x7F
#define ESCAPE_KEY 0x1B

// so we can process CTRL + (SOME KEY)
#define CTRL_KEY(k) ((k) & 0x1f)

void processKeypresses(uint32_t character, Editor* editor);
void appChar(Editor* editor, size_t pos, uint32_t character);
void checkCursorLines(Editor* editor, char c);
void delChar(Editor* editor, size_t pos);
void appendLine(Row* row, uint32_t* s, size_t len);
void removeLine(int pos, Editor* editor);
void deleteChar(Editor* editor);
void positionCursor(Editor* editor, App* a);
void scroll(Editor* editor);
int calcCursorY(Editor* editor);
void tabChange(Editor* editor, int pos);
int isTab(Editor* editor, size_t pos, int y);
int getTabCount(Editor* editor, int y);
int countTabX(Editor* editor, size_t end, bool countX);
void checkAheadTab(Editor* editor, size_t xBeforeNL, size_t xAfterNL, int rF);
int roundXToTabSpace(unsigned char tabcount, int num);
void checkCharacterWith(Editor* editor);

#endif /* KEYPRESSES_H_ */