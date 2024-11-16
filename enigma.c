#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



// Osar.romero - Marc.marza



#include "struct_definitions.h"
#include "utils/io_utils.h"

// This is the client
 Harley_enigma enigma;





void closeFds() {
    // ESTO SERA PARA CERRAR LOS SOCKETS CUANDO ESTEN QUE SERAN GLOBALES
}

void freeMemory() {
    free(enigma.gotham_ip);
    free(enigma.fleck_ip);
    free(enigma.folder);
    free(enigma.worker_type);
}


 void closeProgramSignal() {
    freeMemory();
    closeFds();
    exit(0);
}

void saveEnigma(char *filename) {
    int data_file_fd = open(filename, O_RDONLY);
    if (data_file_fd < 0) {
        printError("Error while opening the Fleck file");
        exit(1);
    }

    enigma.gotham_ip = readUntil('\n', data_file_fd);
    enigma.gotham_port = atoi(readUntil('\n', data_file_fd));

    enigma.fleck_ip = readUntil('\n', data_file_fd);
    enigma.fleck_port = atoi(readUntil('\n', data_file_fd));

    enigma.folder = readUntil('\n', data_file_fd);
    enigma.worker_type = readUntil('\n', data_file_fd);

}




void initalSetup(int argc) {
    if (argc < 2) {
        printError("ERROR: Not enough arguments provided\n");
        exit(1);
    }
    signal(SIGINT, closeProgramSignal);
}






int main(int argc, char *argv[]) {
    initalSetup(argc);

    saveEnigma(argv[1]);

    freeMemory();
    return 0;
}