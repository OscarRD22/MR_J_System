#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "struct_definitions.h"
#include "utils/io_utils.h"

// This is the client
 Harley_enigma harley;

void saveHarley(char *filename) {
    int data_file_fd = open(filename, O_RDONLY);
    if (data_file_fd < 0) {
        printError("Error while opening the Fleck file");
        exit(1);
    }

    harley.gotham_ip = readUntil('\n', data_file_fd);
    harley.gotham_port = atoi(readUntil('\n', data_file_fd));

    harley.fleck_ip = readUntil('\n', data_file_fd);
    harley.fleck_port = atoi(readUntil('\n', data_file_fd));

    harley.folder = readUntil('\n', data_file_fd);
    harley.worker_type = readUntil('\n', data_file_fd);

}

void closeProgramSignal() {
    freeMemory();
    closeFds();
    exit(0);
}

void closeFds() {
    // ESTO SERA PARA CERRAR LOS SOCKETS CUANDO ESTEN QUE SERAN GLOBALES
}

void initalSetup(int argc) {
    if (argc < 2) {
        printError("ERROR: Not enough arguments provided\n");
        exit(1);
    }
    signal(SIGINT, closeProgramSignal);
}


void freeMemory() {
    free(harley.gotham_ip);
    free(harley.fleck_ip);
    free(harley.folder);
    free(harley.worker_type);
}


int main(int argc, char *argv[]) {
    initalSetup(argc);

    saveHarley(argv[1]);

    freeMemory();
    return 0;
}