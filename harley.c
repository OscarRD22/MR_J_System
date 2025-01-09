#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils/utils_connect.h"
#include "struct_definitions.h"
#include "utils/io_utils.h"
#include "so_compression.h"
#include <time.h>
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

int connectHarleyToGotham()
{
    // Usar la variable global gothamSocketFD
    gothamSocketFD = createAndConnectSocket(harley.gotham_ip, harley.gotham_port, FALSE);
    if (gothamSocketFD < 0)
    {
        printError("Error al crear y conectar el socket con Gotham");
        return -1;
    }

    // Crear el mensaje a enviar a Gotham
    char *messageBuffer = NULL;
    if (asprintf(&messageBuffer, "%s&%s&%d", harley.worker_type, harley.gotham_ip, harley.gotham_port) < 0)
    {
        printError("Error al asignar memoria para el mensaje");
        return -1;
    }

    SocketMessage messageToSend = {
        .type = 0x02,
        .dataLength = strlen(messageBuffer),
        .data = strdup(messageBuffer),
        .timestamp = (unsigned int)time(NULL),
        .checksum = calculateChecksum(messageBuffer, strlen(messageBuffer))};

    free(messageBuffer);

    sendSocketMessage(gothamSocketFD, messageToSend);
    free(messageToSend.data);

    // Recibir la respuesta de Gotham
    SocketMessage response = getSocketMessage(gothamSocketFD);
    if (response.type != 0x02 || (response.dataLength > 0 && strcmp(response.data, "CON_KO") == 0))
    {
        printError("Conexión rechazada por Gotham");
        free(response.data);
        return -1;
    }

    free(response.data);
    // printToConsole("Connected to Gotham successfully.\n");

    return 0; // Éxito
}

void simulateDistortionProcess(int clientSocketFD)
{
    printToConsole("Receiving original media/text...");
    sleep(2); // Simula procesamiento

    printToConsole("Distorting...");
    sleep(2); // Simula distorsión

    printToConsole("Sending distorted data back...");
    SocketMessage response = {
        .type = 0x12, // Respuesta exitosa
        .dataLength = strlen("DISTORT_OK"),
        .data = strdup("DISTORT_OK"),
        .timestamp = (unsigned int)time(NULL),
        .checksum = calculateChecksum("DISTORT_OK", strlen("DISTORT_OK"))};

    sendSocketMessage(clientSocketFD, response);
    free(response.data);

    printToConsole("Distortion process completed. Disconnecting...\n");

    // Cerrar el socket después del proceso
    close(clientSocketFD);

    // Notificar a Gotham que el worker está disponible
    //releaseWorker(harley.gotham_ip, harley.gotham_port);
}



int main(int argc, char *argv[])
{
    initalSetup(argc);
    printToConsole("Reading configuration file.\n");
    saveHarley(argv[1]);
    printToConsole("Connecting Harley worker to the system...");

    while (1)
    {
        printToConsole("Connected to Mr. J System, ready to listen to Fleck petitions\n");
        printToConsole("\nWaiting for connections...\n");
        // Intentar conexión a Gotham
        if (connectHarleyToGotham() != 0)
        {
            printError("Failed to connect to Gotham. Retrying in 5 seconds...\n");
            sleep(2);
            continue;
        }

        // printToConsole("Connected to Mr. J System, ready to listen to Fleck petitions\n");

        // Mantener la conexión y procesar mensajes
        while (1)
        {
            // printToConsole("Waiting for connections...\n");
            SocketMessage message = getSocketMessage(gothamSocketFD);

            // Simulación: Espera un mensaje desde Gotham/Fleck
            if (message.data != NULL && message.type == 0x10)
            {
                // Procesar los mensajes según el tipo
                simulateDistortionProcess(gothamSocketFD);
                free(message.data);
            }

            
        }
    }

    closeProgramSignal();
    return 0;
}
