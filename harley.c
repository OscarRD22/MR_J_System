#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils/utils_connect.h"
#include "struct_definitions.h"
#include "utils/io_utils.h"

// Osar.romero - Marc.marza

// This is the client
Harley_enigma harley;
int gothamSocketFD;

/**
 * @brief Free the memory allocated
 */
void freeMemory()
{
    free(harley.gotham_ip);
    free(harley.fleck_ip);
    free(harley.folder);
    free(harley.worker_type);
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
 * @brief Saves the information of the Harley file into the Harley struct
 */
void saveHarley(char *filename)
{
    int data_file_fd = open(filename, O_RDONLY);
    if (data_file_fd < 0)
    {
        printError("Error while opening the Harley file");
        exit(1);
    }

    harley.gotham_ip = readUntil('\n', data_file_fd);
    harley.gotham_port = atoi(readUntil('\n', data_file_fd));

    harley.fleck_ip = readUntil('\n', data_file_fd);
    harley.fleck_port = atoi(readUntil('\n', data_file_fd));

    harley.folder = readUntil('\n', data_file_fd);
    harley.worker_type = readUntil('\n', data_file_fd);
}

/**
 * @brief Closes the program correctly cleaning the memory and closing the file descriptors
 */
void closeProgramSignal()
{
    printToConsole("\nClosing program Harley\n");
    freeMemory();
    closeFds();
    exit(0);
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
 * @brief Connects to the Gotham server
 */
void connectToGotham()
{
    // Creado y conectado a Gotham
    if ((gothamSocketFD = createAndConnectSocket(harley.gotham_ip, harley.gotham_port, FALSE)) < 0)
    {
        printError("Error connecting to Gotham\n");
        exit(1);
    }

    // CONNECTED TO GOTHAM
    SocketMessage m;
    char *buffer;
    asprintf(&buffer, "%s&%s&%d&%s&%d&%s&%s", harley.fleck_ip, harley.gotham_ip, harley.gotham_port, harley.fleck_ip, harley.fleck_port, harley.folder, harley.worker_type);
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
 * @brief Main function of the Harley server
 * @param argc The number of arguments
 * @param argv The arguments
 * @return 0 if the program ends correctly
 */
int main(int argc, char *argv[])
{
    initalSetup(argc);

    saveHarley(argv[1]);

    connectToGotham();
    closeProgramSignal();
    return 0;
}