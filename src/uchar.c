#define _XOPEN_SOURCE
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <wchar.h>
#include <stdio.h>
#include <inttypes.h>

#include "uchar.h"
#include "keypresses.h"
#include "structs.h"
#include "display.h"
#include "error.h"

// convert a singular char to utf-8
uint32_t chartouint(const char* str) {
    const unsigned char* ptr = (unsigned char*)str;
    uint32_t ret = (uint32_t)*ptr;
    ptr++;
    int shftby = 8;
    for (int i = 0; i < mblen(str, MB_CUR_MAX) - 1; i++) {
        ret |= (uint32_t)*ptr << shftby;
        ptr++;
        shftby += 8;
    }
    return ret;
}

// converts a multibyte char array into a uint32_t array
// arrLen is the actual length of the array
uint32_t* mbtoutf(const char* str, size_t length, size_t* arrLen) {
    const char* ptr = str;
    const char* final = str + length;
    size_t index = 0;
    uint32_t* ret = check_malloc((length + 1) * sizeof(uint32_t));
    while (ptr < final) {
        int indLen = mblen(ptr, MB_CUR_MAX);
        if (indLen > 0) {
            ret[index] = chartouint(ptr);
            ptr += indLen;
        }
        else break;
        index++;
    }
    ret[index] = '\0';
    if (arrLen) *arrLen = index;
    return ret;
}

#define ASCII_LIMIT 0x7F
#define TWO_BIT_LIMIT 0xDFBF
#define THREE_BIT_LIMIT 0xEFBFBF
#define FOUR_BIT_LIMIT 0xF48FBFBF
#define UTF16_SURROGATE_LIMIT 0xEDBFBF

// gets length of utf-8 character (trivial, but still works)
char getLenUTF(uint32_t c) {
    return 1 + (c >= ASCII_LIMIT) + (c >= TWO_BIT_LIMIT) + (c >= THREE_BIT_LIMIT) + (c >= FOUR_BIT_LIMIT);
}

// reverse byte order of a given character
uint32_t reverseBytes(uint32_t character, char length) {
    uint32_t ret = 0;
    uint8_t byte;
    int i;

    for(i = 0; i < length * 8; i += 8)
    {
        byte = (character >> i) & 0xff;
        ret |= byte << (length * 8 - 8 - i);
    }
    return ret;
}

// check if a given utf8 value is valid
int isValidUTF(uint32_t character) {
    if (character <= 0x7F) return 1; // if ASCII, return true

    // make sure we are using the correct byte order
    unsigned int i = 1;
    uint32_t c;
    if ((*(char*)&i) == 1) c = reverseBytes(character, getLenUTF(character));
    else c = character;

    if (0xC280 <= c && c <= TWO_BIT_LIMIT) return ((c & 0xE0C0) == 0xC080);
    if (0xEDA080 <= c && c <= UTF16_SURROGATE_LIMIT) return 0;
    if (0xE0A080 <= c && c <= THREE_BIT_LIMIT) return ((c & 0xF0C0C0) == 0xE08080);
    if (0xF0908080 <= c && c <= FOUR_BIT_LIMIT) return ((c & 0xF8C0C0C0) == 0xF0808080);
    return 0;
}

// convert a uint32_t array to a char array
char* utftomb(const uint32_t* str, size_t length, size_t* arrLen) {
    const uint32_t* ptr = str;
    const uint32_t* final = str + length;
    char* ret = check_malloc((length + 1) * sizeof(uint32_t));
    size_t index = 0;
    while (ptr < final || *ptr == 0x03) {
        char lengthOfMB = getLenUTF(*ptr);
        memcpy(&ret[index], ptr, lengthOfMB);
        index += lengthOfMB;
        ptr++;
    }
    if (arrLen) *arrLen = index;
    ret[index] = '\0';
    return ret;
}

// gets the keypresses from the keyboard and returns a 
// character containing the keypress
uint32_t getMKeys() {
    int n;
    uint32_t character = '\0';
    if ((n = read(STDIN_FILENO, &character, sizeof(uint32_t))) < 1) {
        if (n == -1 && errno != EAGAIN) errorHandle("Velte: read");
    }
    
    // checking for arrow keys
    if (memcmp(&character, "\033[A", 3) == 0)
        character = ARROW_UP;
    else if (memcmp(&character, "\033[B", 3) == 0)
        character = ARROW_DOWN;
    else if (memcmp(&character, "\033[C", 3) == 0)
        character = ARROW_RIGHT;
    else if (memcmp(&character, "\033[D", 3) == 0)
        character = ARROW_LEFT;
    else if (!isValidUTF(character)) return 0;

    return character;
}

// see how many cells a character takes up on the command line
char getCharWidth(uint32_t ch) {
    return wcwidth(ch);
}

#define ONE_BYTE_MASK 0x7F
#define TWO_BYTE_MASK 0x1F
#define THREE_BYTE_MASK 0x0F
#define FOUR_BYTE_MASK 0x07
#define MASK_BY 0x3F

// convert a utf-8 sequence into a utf-32 unicode codepoint
uint32_t utfToCodepoint(const char* in, char chLen) {
    uint32_t ret;
    char mask;
    switch (chLen) {
        case 1:
            mask = ONE_BYTE_MASK;
            break;
        case 2:
            mask = TWO_BYTE_MASK;
            break;
        case 3:
            mask = THREE_BYTE_MASK;
            break;
        case 4:
            mask = FOUR_BYTE_MASK;
            break;
        default:
            return -1;
    }
    ret = (in[0] & mask);
    for (int i = 1; i < chLen; i++) {
        ret <<= 6;
        ret |= (in[i] & MASK_BY);
    }
    return ret;
}

void calculateCharacterWidth(Editor* editor, uint32_t ch, int* utfJump) {
    if (editor->config.isUtf8) {
        if (ch == '\t') return;
        char* toStr = utftomb(&ch, 1, NULL);
        *utfJump += getCharWidth(utfToCodepoint(toStr, getLenUTF(ch))) - 1;
    }
}