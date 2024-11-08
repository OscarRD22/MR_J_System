#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../struct_definitions.h"

void printToConsole(char *x) {
    write(1, x, strlen(x));
    fflush(stdout);
}

void printError(char *x) {
    write(2, x, strlen(x));
    fflush(stderr);
}

// CREO QUE ESTO TIENE MEMORY LEAK EN ALGUN PUNTO REVISAR
char *readUntil(char del, int fd) {
    char *chain = malloc(sizeof(char) * 1);
    char c;
    int i = 0, n;

    n = read(fd, &c, 1);

    while (c != del && n != 0 && n != -1) {
        chain[i] = c;
        i++;
        char *temp = realloc(chain, (sizeof(char) * (i + 1)));
        if (temp == NULL) {
            printError("Error reallocating memory\n");
            free(chain);
            return NULL;
        }
        chain = temp;
        n = read(fd, &c, 1);
    }

    chain[i] = '\0';

    return chain;
}

void validateString(char *str) {
    if (strchr(str, '&') != NULL) {
        printError("WARNING: String containts &\n");
        char *newStr = malloc((strlen(str) + 1) * sizeof(char));
        int j = 0;
        for (size_t i = 0; i < strlen(str); i++) {
            if (str[i] != '&') {
                newStr[j] = str[i];
                j++;
            }
        }
        newStr[j] = '\0';
        free(str);
        str = strdup(newStr);
        free(newStr);
    }
}

void printArray(char *array) {
    printToConsole("Printing array\n");
    char *buffer;
    for (size_t i = 0; i < strlen(array); i++) {
        asprintf(&buffer, "{%c}\n", array[i]);
        printToConsole(buffer);
        free(buffer);
    }
    printToConsole("Finished printing array\n");
}