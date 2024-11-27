#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Osar.romero - Marc.marza

#include "struct_definitions.h"
#include "utils/io_utils.h"
#include "utils/utils_connect.h"

// This is the client
Harley_enigma enigma;
int gothamSocketFD;


/**
 * @brief Free the memory allocated
 */
void freeMemory()
{
    free(enigma.gotham_ip);
    free(enigma.fleck_ip);
    free(enigma.folder);
    free(enigma.worker_type);
}

/**
 * @brief Closes the file descriptors if they are open
 */
void closeFds()
{
    if (gothamSocketFD > 0)
    {
        close(gothamSocketFD);
    }
}

/**
 * @brief Closes the program correctly cleaning the memory and closing the file descriptors
 */
void closeProgramSignal()
{
    printToConsole("\nClosing program Enigma\n");
    freeMemory();
    closeFds();
    exit(0);
}

/**
 * @brief Saves the information of the Enigma file into the Enigma struct
 *  @param filename The name of the file to read
 */
void saveEnigma(char *filename)
{
    int data_file_fd = open(filename, O_RDONLY);
    if (data_file_fd < 0)
    {
        printError("Error while opening the Enigma file");
        exit(1);
    }

    enigma.gotham_ip = readUntil('\n', data_file_fd);
    enigma.gotham_port = atoi(readUntil('\n', data_file_fd));

    enigma.fleck_ip = readUntil('\n', data_file_fd);
    enigma.fleck_port = atoi(readUntil('\n', data_file_fd));

    enigma.folder = readUntil('\n', data_file_fd);
    enigma.worker_type = readUntil('\n', data_file_fd);
}

/**
 * @brief Checks if the number of arguments is correct
 * @param argc The number of arguments
 */
void initalSetup(int argc)
{
    if (argc < 2)
    {
        printError("ERROR: Not enough arguments provided\n");
        exit(1);
    }
    signal(SIGINT, closeProgramSignal);
}

/**
 * @brief Connects to Gotham
 */
void connectToGotham()
{
    // Creado y conectado a Gotham
    if ((gothamSocketFD = createAndConnectSocket(enigma.gotham_ip, enigma.gotham_port, FALSE)) < 0)
    {
        printError("Error connecting to Gotham\n");
        exit(1);
    }

    // CONECTADO TO GOTHAM
    SocketMessage m;
    char *buffer;
    asprintf(&buffer, "%s&%s&%d&%s&%d&%s&%s", enigma.fleck_ip, enigma.gotham_ip, enigma.gotham_port, enigma.fleck_ip, enigma.fleck_port, enigma.folder, enigma.worker_type);
    m.type = 0x02;
    m.dataLength = strlen(buffer);
    m.data = strdup(buffer);
    m.timestamp = (unsigned int)time(NULL);
    m.checksum = calculateChecksum(buffer, strlen(buffer));
    sendSocketMessage(gothamSocketFD, m);
    free(buffer);
    free(m.data);
    close(gothamSocketFD);

    // Receive response
    SocketMessage response = getSocketMessage(gothamSocketFD);

    // handle response
    free(response.data);
    close(gothamSocketFD);
}

/**
 * @brief Main function of the Enigma server
 * @param argc The number of arguments
 * @param argv The arguments
 * @return 0 if the program ends correctly
 */
int main(int argc, char *argv[])
{
    initalSetup(argc);
    saveEnigma(argv[1]);
    connectToGotham();
    // No hace falta porque ya lo hace en la funcion closeProgramSignal()
    // freeMemory();
    closeProgramSignal();
    return 0;
}