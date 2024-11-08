#ifndef STRUCT_DEFINITIONS_H
#define STRUCT_DEFINITIONS_H

#include <stdint.h>

#define TRUE 1
#define FALSE 0

typedef struct {
    char *username;
    char *folder;
    char *ip;
    int port;
} Fleck;

typedef struct {
    char *fleck_ip;
    int fleck_port;
    char *harley_enigma_ip;
    int harley_enigma_port;
} Gotham;

typedef struct {
    char *gotham_ip;
    int gotham_port;
    char *fleck_ip;
    int fleck_port;
    char *folder;
    char *worker_type;
} Harley_enigma;

#endif