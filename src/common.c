#include "common.h"

#include <stdio.h>
#include <stdlib.h>

void check_malloc(void *pointer) {
    if (!pointer) {
        perror("Malloc failed!\n");
        exit(EXIT_FAILURE);
    }
}