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
void logout();
void clearAll();
void handleDistortCommand(const char *filename, const char *factor);
void sendFileToWorker(int workerSocketFD, const char *filename);
void handleResumeCommand(const char *filename);
void checkStatus();
