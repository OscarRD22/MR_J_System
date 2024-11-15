#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "struct_definitions.h"
#include "utils/io_utils.h"


// Osar.romero - Marc.marza



// This is the client
 Gotham gotham;
 int data_file_fd, listenToFleck,listenToEnigma, listenToHarley = 0;
 pthread_t FleckThread, EnigmaThread, HarleyThread;


void saveGotham(char *filename) {
    data_file_fd = open(filename, O_RDONLY);
    if (data_file_fd < 0) {
        printError("Error while opening the Fleck file");
        exit(1);
    }

   gotham.fleck_ip = readUntil('\n', data_file_fd);
    char *port1 = readUntil('\n', data_file_fd);
    gotham.fleck_port = atoi(port1);
    free(port1);

    gotham.harley_enigma_ip = readUntil('\n', data_file_fd);
    char *port = readUntil('\n', data_file_fd);
    gotham.harley_enigma_port = atoi(port);
    free(port);

}

void closeProgramSignal() {
    //freeMemory();
    //closeFds();
    exit(0);
}
/**
void closeFds() {
    // ESTO SERA PARA CERRAR LOS SOCKETS CUANDO ESTEN QUE SERAN GLOBALES
}
*/
void initalSetup(int argc) {
    if (argc < 2) {
        printError("ERROR: Not enough arguments provided\n");
        exit(1);
    }
    signal(SIGINT, closeProgramSignal);
}


void freeMemory() {
    free(gotham.fleck_ip);
    free(gotham.harley_enigma_ip);
}


int main(int argc, char *argv[]) {
    initalSetup(argc);

    saveGotham(argv[1]);

if (pthread_create(&FleckThread, NULL, (void *)listenToFleck, NULL) != 0) {
        printError("Error creating Fleck thread\n");
        exit(1);
    }

if (pthread_create(&EnigmaThread, NULL, (void *)listenToEnigma, NULL) != 0) {
        printError("Error creating Enigma thread\n");
        exit(1);
    }

    if (pthread_create(&HarleyThread, NULL, (void *)listenToHarley, NULL) != 0) {
        printError("Error creating Harley thread\n");
        exit(1);
    }



    printToConsole("Gotham saved\n");

    freeMemory();
    return 0;
}



/**
 * @brief Closes the file descriptors if they are open
 */
void closeFds() {
    if (data_file_fd != 0) {
        close(data_file_fd);
    }
    if (listenToFleck != 0) {
        close(listenToFleck);
    }
    if (EnigmaThread != 0) {
        close(EnigmaThread);
    }
    if (HarleyThread != 0) {
        close(HarleyThread);
    }
}