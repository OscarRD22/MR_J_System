#ifndef STRUCT_DEFINITIONS_H
#define STRUCT_DEFINITIONS_H

#include <stdint.h>

// Osar.romero - Marc.marza
// ssh oscar.romero@montserrat.salle.url.edu
#define TRUE 1
#define FALSE 0

typedef struct
{
    uint8_t type;
    uint16_t dataLength;
    char *data;
    uint16_t checksum;
    uint32_t timestamp;
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
    char *workerName;
    char *workerType;
    char *WorkerIP;
    int WorkerPort;

} WorkerServer;

#endif