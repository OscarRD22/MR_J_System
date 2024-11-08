#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>

void printToConsole(char *x);
void printError(char *x);
char *readUntil(char del, int fd);
void printArray(char *array);
void validateString(char *str);