#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "struct_definitions.h"
#include "utils/io_utils.h"
#include "utils/utils_fleck.h"


// Osar.romero - Marc.marza


// This is the client
Fleck fleck;
char *command;

void saveFleck(char *filename) {
    int data_file_fd = open(filename, O_RDONLY);
    if (data_file_fd < 0) {
        printError("Error while opening the Fleck file");
        exit(1);
    }
    fleck.username = readUntil('\n', data_file_fd);
    validateString(fleck.username);
    fleck.folder = readUntil('\n', data_file_fd);
    fleck.ip = readUntil('\n', data_file_fd);
    char *port = readUntil('\n', data_file_fd);
    fleck.port = atoi(port);
    free(port);

    char *buffer;
    asprintf(&buffer, "%s user has been initialized\n$ ", fleck.username);
    printToConsole(buffer);
    free(buffer);
}

/**
 * @brief Free the memory allocated 
 */
void freeMemory() {
    free(fleck.username);
    free(fleck.folder);
    free(fleck.ip);
    if (command != NULL) {
        free(command);
    }
}

void closeFds() {
    // ESTO SERA PARA CERRAR LOS SOCKETS CUANDO ESTEN QUE SERAN GLOBALES
}

void closeProgramSignal() {
    freeMemory();
    closeFds();
    exit(0);
}

void initalSetup(int argc) {
    if (argc != 2) {
        printError("ERROR: Not enough arguments provided\n");
        exit(1);
    }
    signal(SIGINT, closeProgramSignal);
}


void commandInterpretter() {
    int bytesRead;
    command = NULL;
    int continueReading = TRUE, CONNECTED = FALSE;

    do {
        if (command != NULL) {
            free(command);
            command = NULL;
        }
        command = readUntil('\n', 0);

        bytesRead = strlen(command);

        if (bytesRead == 0) {
            printError("ERROR NO BYTES READ");
            break;
        }
        // FORMAT THE COMMAND ADDING THE \0 AND REMOVING THE \n
        int len = strlen(command);
        if (len > 0 && command[len - 1] == '\n') {
            command[len - 1] = '\0';
        }
        if (strcasecmp(command, "CONNECT") == 0) {
            char *buffer = NULL;
            asprintf(&buffer, "%s connected to Mr. J System. Let the chaos begin!:)\n", fleck.username);
            printToConsole(buffer);
            free(buffer);            
        
             connectToGotham(FALSE);
             free(command);
             command = NULL;
            // Vienen una ip con server de Harley/Enigma
            // conectToEnigma();
        } else if (strcasecmp(command, "LOGOUT") == 0) {
            printToConsole("Thanks for using Mr. J System, see you soon, chaos lover :)\n");
            logout();  // DESCONECTAR SOCKETS Y SALIR
            continueReading = FALSE;
        } else {  // COMMAND HAS MORE THAN ONE WORD
            char *token = strtok(command, " ");
            if (token != NULL) {
                if (strcasecmp(token, "DISTORT")==0) {
                    if(CONNECTED == FALSE){
                        printError("Cannot distort, you are not connected to Mr. J System\n");
                        free(command);
                        command = NULL;
                    }else{
                    printf("ESTE ES EL TOKEN %s\n", token);

                    printToConsole("Command ok\n");
                    }
                 
                }else if (strcasecmp(token,"CHECK")==0){
                    token = strtok(NULL, " ");
                    if (token != NULL && strcmp(token, "STATUS") == 0) {
                        printToConsole("Command ok\n");
                    }else {
                        printError("You have no ongoing or finished distorsions\n");
                        free(command);
                        command = NULL;
                    }
                }else if (strcasecmp(token,"LIST")==0){
                    token = strtok(NULL, " ");
                    //F1 SE TIENE QUE IMPLEMENTAR
                    if (token != NULL && strcasecmp(token, "MEDIA") == 0) {
                        listMedia();
                    }else if(token != NULL && strcasecmp(token, "TEXT") == 0){
                        listText();
                    }else{
                        printError("Unknown command\n");
                        free(command);
                        command = NULL;
                    }
                }else{
                    printError("Unknown command\n");
                    free(command);
                    command = NULL;
                }
                
            } else {
                printError("ERROR: Please input a valid command.\n");
                free(command);
                command = NULL;
            }
        }
        printToConsole("$ ");
    } while (continueReading == TRUE);
    free(command);
    command = NULL;
    }

int main(int argc, char *argv[]) {
    initalSetup(argc);

    saveFleck(argv[1]);

    commandInterpretter();
    freeMemory();
    return 0;
}