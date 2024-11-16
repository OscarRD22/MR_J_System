#define _GNU_SOURCE
#include <stdio.h>

#include "io_utils.h"
#include "../struct_definitions.h"


// Osar.romero - Marc.marza


SocketMessage getSocketMessage(int clientFD);
int createAndConnectSocket(char *IP, int port, int isVerbose);
int createAndListenSocket(char *IP, int port);
void sendSocketMessage(int socketFD, SocketMessage message);

