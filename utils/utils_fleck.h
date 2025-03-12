#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include "../struct_definitions.h"

// Osar.romero - Marc.marza

extern int gothamSocketFD, distorsionSocketFD;
extern Fleck fleck;

void listMedia();
void listText();
int connectToGotham(int isExit);
void clearAll();
void handleDistortCommand(char *fullPath, char *filename,  char *factor);
void sendFileToWorker(int workerSocketFD,  char *filename);
void checkStatus();
