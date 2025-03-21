#define _GNU_SOURCE
#include <stdio.h>

#include "io_utils.h"
#include "../struct_definitions.h"

// Osar.romero - Marc.marza

SocketMessage getSocketMessage(int clientFD);
int createAndConnectSocket(char *IP, int port, int isVerbose);
int createAndListenSocket(char *IP, int port);
void sendSocketMessage(int socketFD, SocketMessage message);
int calculateChecksum(const char *buffer, size_t length);
void sendError(int socketFD);
int compareMD5Sum(char *path, char *md5sum);
void sendFile(int socketFD, char *filename);
void receiveFile(int socketFD, char *filename);
