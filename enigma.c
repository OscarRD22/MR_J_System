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

int connectToGothamWorker(const char *workerType, const char *ip, int port)
{
    

    // Crear y conectar el socket a Gotham
    if ((gothamSocketFD = createAndConnectSocket(ip, port, FALSE)) < 0)
    {
        printError("Error connecting to Gotham\n");
        exit(1);
    }

    // Preparar el mensaje de solicitud de conexión
    SocketMessage m;
    m.type = 0x02; // Tipo de mensaje para solicitar conexión
    char *buffer;
    m.dataLength = strlen(buffer);
    asprintf(&buffer, "%s&%s&%d", workerType, ip, port);
    m.data = strdup(buffer);
    // m.timestamp = (unsigned int)time(NULL);
    // m.checksum = calculateChecksum(m.data, m.dataLength);
    // Enviar el mensaje de solicitud a Gotham
    sendSocketMessage(gothamSocketFD, m);
    free(m.data);
    free(buffer);

    // Recibir la respuesta de Gotham
    SocketMessage response = getSocketMessage(gothamSocketFD);

    if (response.type == 0x02)
    {
        if (response.dataLength == 0)
        {
            printToConsole("Connected to Mr. J System, ready to listen to Fleck petitions\n\n");
            printToConsole("Waiting for connections...\n");
        }
        else if (response.dataLength == strlen("CON_KO") && strcmp(response.data, "CON_KO") == 0)
        {
            printError("Error: Unable to establish connection with Gotham.\n");
            free(response.data);
            close(gothamSocketFD);
            return -1; // Retorna un error si la conexión falla
        }
    }
    else
    {
        printError("Unexpected response type from Gotham.\n");
        free(response.data);
        close(gothamSocketFD);
        return -1;
    }

    // Liberar memoria y cerrar el socket
    free(response.data);
    close(gothamSocketFD);

    return 0; // Retorna 0 si la conexión fue exitosa
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

    printToConsole("Reading configuration file.\n");

    saveEnigma(argv[1]);

    printToConsole("Connecting Enigma worker to the system...\n");

    // Conectar Enigma a Gotham
    if (connectToGothamWorker("Enigma", enigma.gotham_ip, enigma.gotham_port) != 0)
    {
        // Manejar el error de conexión
        return 1;
    }

    while (1)
    {
        // Código para recibir y manejar peticiones de clientes
    }

    closeProgramSignal();
    return 0;
}