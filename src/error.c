#include "error.h"

#include <stdio.h>
#include <stdlib.h>

// error handling
void errorHandle(const char* err) {
    perror(err);
    exit(1);
}