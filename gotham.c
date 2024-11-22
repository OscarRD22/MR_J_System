#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "struct_definitions.h"
#include "utils/io_utils.h"
#include "utils/utils_connect.h"

// Osar.romero - Marc.marza

// This is the client
Gotham gotham;
int data_file_fd, listenFleckFD, listenEnigmaFD, listenHarleyFD = 0;
pthread_t FleckThread, DistorsionWorkersThread;
int terminate = FALSE;
int numOfWorkers = 0;

//
WorkerServer *workers;// Array of workers

/**
 * @brief Saves the information of the Gotham file into the Gotham struct
 *  @param filename The name of the file to read
 */
void saveGotham(char *filename)
{
    data_file_fd = open(filename, O_RDONLY);
    if (data_file_fd < 0)
    {
        printError("Error while opening the Fleck file\n");
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
    printToConsole("\n\nReading configuration file\n\n");
}

/**
 * @brief Closes the file descriptors if they are open
 */
void closeFds()
{
    if (data_file_fd != 0)
    {
        close(data_file_fd);
    }
    if (listenFleckFD != 0)
    {
        close(listenFleckFD);
    }
    if (listenEnigmaFD != 0)
    {
        close(listenEnigmaFD);
    }
    if (listenHarleyFD != 0)
    {
        close(listenHarleyFD);
    }
}
/**
 * @brief Frees the memory allocated for the Gotham struct
 */
void freeMemory()
{
    free(gotham.fleck_ip);
    free(gotham.harley_enigma_ip);
}
/**
 * @brief Closes the program correctly cleaning the memory and closing the file descriptors
 */
void closeProgramSignal()
{
    printToConsole("\nClosing program\n");
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
 * @brief Listens to the Fleck server and sends the information of the workers server with less Flecks connected
 */
void *listenToFleck()
{
    printToConsole("Listening to Fleck\n\n");

    if ((listenFleckFD = createAndListenSocket(gotham.fleck_ip, gotham.fleck_port)) < 0)
    {
        printError("Error creating Fleck socket\n");
        exit(1);
    }

    while (terminate == FALSE)
    {

        int fleckSocketFD = accept(listenFleckFD, (struct sockaddr *)NULL, NULL);

        if (fleckSocketFD < 0)
        {
            printError("Error accepting Fleck\n");
            exit(1);
        }

        printToConsole("Fleck connected\n");

        SocketMessage m = getSocketMessage(fleckSocketFD);
        printToConsole("Message received\n");

        if (m.type == 0x01)
        {
            printToConsole("New Fleck connected to Gotham\n");
        }
    }
    return NULL;
}

/**
 * @brief Listens to the Workers distorsion server Enigma/Harley and adds it to the list
 */
void *listenToDistorsionWorkers()
{
    printToConsole("Listening to WORKERS\n\n");

    if ((listenEnigmaFD = createAndListenSocket(gotham.harley_enigma_ip, gotham.harley_enigma_port)) < 0)
    {
        printError("Error creating workers socket\n");
        exit(1);
    }

    while (terminate == FALSE)
    {

        // ESTO ES BLOCKING
        int workerSocketFD = accept(listenEnigmaFD, (struct sockaddr *)NULL, NULL);

        if (workerSocketFD < 0)
        {
            printError("Error accepting worker\n");
            exit(1);
        }

        printToConsole("Worker connected\n");

        // ESTO ES BLOCKING
        SocketMessage m = getSocketMessage(workerSocketFD);

        /* if (m.type == 0x02)
         {
             printToConsole("New worker connected to Gotham\n");
         }
         */
    }

    return NULL;
}

/**
 * @brief Main function of the Gotham server
 * @param argc The number of arguments
 * @param argv The arguments
 * @return 0 if the program ends correctly
 */
int main(int argc, char *argv[])
{
    initalSetup(argc);

    saveGotham(argv[1]);
    printToConsole("Gotham server initialized\n\n");
    printToConsole("Waiting for connections...\n\n");

    // Retorna 0 si tiene éxito o un código de error si falla
    if (pthread_create(&FleckThread, NULL, (void *)listenToFleck, "Thread Fleck creado\n") != 0)
    {
        printError("Error creating Fleck thread\n");
        exit(1);
    }

    if (pthread_create(&DistorsionWorkersThread, NULL, (void *)listenToDistorsionWorkers, NULL) != 0)
    {
        printError("Error creating Enigma thread\n");
        exit(1);
    }

    pthread_join(FleckThread, NULL);
    pthread_join(DistorsionWorkersThread, NULL);

    freeMemory();
    return 0;
}
