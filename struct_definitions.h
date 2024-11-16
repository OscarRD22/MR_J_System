#ifndef STRUCT_DEFINITIONS_H
#define STRUCT_DEFINITIONS_H

#include <stdint.h>

// Osar.romero - Marc.marza

#define TRUE 1
#define FALSE 0
#define MAX_WORKERS 20

typedef struct
{
    uint8_t type;
    int dataLength;
    char *data;
    int checksum;
    char timestamp[4];
} SocketMessage;

typedef struct
{
    char *username;
    char *folder;
    char *ip;
    int port;
} Fleck;

typedef struct
{
    char *fleck_ip;
    int fleck_port;
    char *harley_enigma_ip;
    int harley_enigma_port;
} Gotham;

typedef struct
{
    char *gotham_ip;
    int gotham_port;
    char *fleck_ip;
    int fleck_port;
    char *folder;
    char *worker_type;
} Harley_enigma;

typedef struct
{
    int numOfWorkers;
    int WorkerPort;
    char *WorkerIP;
    char *WorkerServername;
    char *Workers[MAX_WORKERS];
} WorkerServer;

#endif