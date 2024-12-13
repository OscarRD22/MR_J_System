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

int connectToEnigmaWorker(const char *WType)
{

    // Crear y conectar el socket a Gotham
    if ((gothamSocketFD = createAndConnectSocket(enigma.gotham_ip, enigma.gotham_port, FALSE)) < 0)
    {
        printError("Error al crear y conectar el socket con Gotham");
        return -1;
    }


// Crear el mensaje a enviar a Gotham
    char *messageBuffer = NULL;
    if (asprintf(&messageBuffer, "%s&%s&%d", enigma.worker_type, enigma.gotham_ip, enigma.gotham_port) < 0)
    {
        printError("Error al asignar memoria para el mensaje");
        close(gothamSocketFD);
        return -1;
    }

    // Imprimir el mensaje que se enviará a Gotham
    printToConsole("Mensaje que envia Enigma a Gotham: ");
    printToConsole(messageBuffer);

    SocketMessage messageToSend;
    messageToSend.type = 0x02; // Tipo de mensaje para conexión
    messageToSend.dataLength = strlen(messageBuffer);
    messageToSend.data = strdup(messageBuffer);
    messageToSend.timestamp = (unsigned int)time(NULL);
    messageToSend.checksum = calculateChecksum(messageToSend.data, messageToSend.dataLength);

    free(messageBuffer); // Liberar memoria del buffer intermedio

    // Enviar el mensaje al servidor Gotham
    sendSocketMessage(gothamSocketFD, messageToSend);

    free(messageToSend.data); // Liberar el mensaje enviado

    // Recibir la respuesta del servidor Gotham
    SocketMessage response = getSocketMessage(gothamSocketFD);
    if (response.type != 0x02)
    {
        printError("Tipo de respuesta inesperado recibido desde Gotham");
        free(response.data);
        close(gothamSocketFD);
        return -1;
    }

    // Verificar el contenido de la respuesta
    if (response.dataLength == strlen("CON_KO") && strcmp(response.data, "CON_KO") == 0)
    {
        printError("Conexión rechazada por Gotham");
        free(response.data);
        close(gothamSocketFD);
        return -1;
    }

    printToConsole("\nConnected to Mr. J System, ready to listen to Fleck petitions\n\n");

    free(response.data);   // Liberar la memoria de la respuesta
    close(gothamSocketFD); // Cerrar el socket si ya no se necesita

    return 0; 

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
    if (connectToEnigmaWorker("Enigma") != 0)
    {
        printError("Failed to connect to Gotham. Exiting...\n");
        closeProgramSignal(0);
    }
    printToConsole("Waiting for connections...\n");
    while (1)
    {
        // Aquí va la lógica principal del servidor
    }

    closeProgramSignal(0);
    return 0;
}