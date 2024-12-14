#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

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
 * @brief Connects to the Gotham server funcion antigua "BORRAR"
 
int connectToGotham()
{
    // Crear y conectar el socket a Gotham
    if ((gothamSocketFD = createAndConnectSocket(harley.gotham_ip, harley.gotham_port, FALSE)) < 0)
    {
        printError("Error connecting to Gotham\n");
        return -1;
    }

    SocketMessage m;
    char *buffer = NULL;

    // Crear el contenido del mensaje
    if (asprintf(&buffer, "%s&%s&%d", "Harley", harley.gotham_ip, harley.gotham_port) < 0)
    {
        printError("Error allocating memory for message\n");
        close(gothamSocketFD); // Cierra el socket antes de salir
        return -1;
    }

    // Configurar los campos del mensaje
    m.type = 0x02;
    m.dataLength = strlen(buffer);
    m.data = strdup(buffer);
    m.timestamp = (unsigned int)time(NULL);
    m.checksum = calculateChecksum(m.data, m.dataLength);

    sendSocketMessage(gothamSocketFD, m);
    free(buffer);

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
            return -1;
        }
    }
    else
    {
        printError("Unexpected response type from Gotham.\n");
        free(response.data);
        close(gothamSocketFD);
        return -1;
    }

    free(response.data);
    close(gothamSocketFD);

    return 0;
}
*/
int connectHarleyToGotham()
{
    int gothamSocketFD = createAndConnectSocket(harley.gotham_ip, harley.gotham_port, FALSE);
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
        close(gothamSocketFD);
        return -1;
    }

    // Imprimir el mensaje que se enviará a Gotham
    printToConsole("Mensaje que envia Harley a Gotham: ");
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
    if (response.type != 0x02 || (response.dataLength && strcmp(response.data, "CON_KO") == 0)) {
        printError("Connection rejected by Gotham.\n");
        free(response.data);
        close(gothamSocketFD);
        return -1;
    }

    printToConsole("\nConnected to Mr. J System, ready to listen to Fleck petitions\n\n");

    free(response.data);   // Liberar la memoria de la respuesta
    close(gothamSocketFD); // Cerrar el socket si ya no se necesita

    return 0; // Éxito
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
    printToConsole("Reading configuration file.\n");
    saveHarley(argv[1]);
    printToConsole("Connecting Harley worker to the system...\n");

    // Conectar Harley a Gotham
    if (connectHarleyToGotham() != 0)
    {
        printError("Failed to connect to Gotham. Exiting...\n");
        closeProgramSignal(0); // Cerrar programa con limpieza
    }
    printToConsole("Waiting for connections...\n");
    // Continuar con la lógica de funcionamiento del servidor
    while (1)
    {
        // Aquí va la lógica principal del servidor
    }

    closeProgramSignal(0); // Limpieza final
    return 0;
}