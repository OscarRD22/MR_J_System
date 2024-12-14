#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "so_compression.h" // Llibreria de compressió
#include "utils/utils_connect.h"
#include "struct_definitions.h"
#include "utils/io_utils.h"
// Osar.romero - Marc.marza

// This is the client
Harley_enigma harley;
int gothamSocketFD;
int isPrimaryWorker = FALSE; // Indica si aquest Harley és el principal
pthread_mutex_t primaryMutex = PTHREAD_MUTEX_INITIALIZER;

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
 * @brief Compress an image using the SO_compression library.
 * @param filename The name of the image file to compress.
 * @param scale_factor The scale factor to apply to the image.
 */
void compressImage(const char *filename, int scale_factor)
{
    printToConsole("Starting image compression...\n");
    int status = SO_compressImage((char *)filename, scale_factor);

    if (status < 0)
    {
        printError("Image compression failed.\n");
        return;
    }

    char message[256];
    snprintf(message, sizeof(message), "Image compression completed successfully for file: %s\n", filename);
    printToConsole(message);
}

/**
 * @brief Compress an audio file using the SO_compression library.
 * @param filename The name of the audio file to compress.
 * @param interval_ms The interval in milliseconds for compression.
 */
void compressAudio(const char *filename, int interval_ms)
{
    printToConsole("Starting audio compression...\n");
    int status = SO_compressAudio((char *)filename, interval_ms);

    if (status < 0)
    {
        printError("Audio compression failed.\n");
        return;
    }

    char message[256];
    snprintf(message, sizeof(message), "Audio compression completed successfully for file: %s\n", filename);
    printToConsole(message);
}

/**
 * @brief Handle a distortion task assigned by Gotham.
 * @param taskType The type of task (image or audio).
 * @param filename The name of the file to process.
 * @param factor The compression factor or interval (depending on task type).
 */
void handleDistortionTask(const char *taskType, const char *filename, int factor)
{
    if (strcasecmp(taskType, "image") == 0)
    {
        compressImage(filename, factor);
    }
    else if (strcasecmp(taskType, "audio") == 0)
    {
        compressAudio(filename, factor);
    }
    else
    {
        printError("Unknown task type. Cannot handle distortion task.\n");
    }
}

/**
 * @brief Handles incoming connections and processes distortion tasks.
 */
void *listenForTasks(void *args)
{
    printToConsole("Listening for incoming tasks...\n");

    int listenFD = createAndListenSocket(harley.fleck_ip, harley.fleck_port);
    if (listenFD < 0)
    {
        printError("Failed to create listening socket.\n");
        pthread_exit(NULL);
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(listenFD, &read_fds);

    int max_fd = listenFD;

    while (1)
    {
        fd_set temp_fds = read_fds;
        int activity = select(max_fd + 1, &temp_fds, NULL, NULL, NULL);

        if (activity < 0)
        {
            printError("Error in select.\n");
            continue;
        }

        if (FD_ISSET(listenFD, &temp_fds))
        {
            int clientSocketFD = accept(listenFD, NULL, NULL);
            if (clientSocketFD < 0)
            {
                printError("Error accepting connection.\n");
                continue;
            }

            SocketMessage request = getSocketMessage(clientSocketFD);

            if (request.type == 0x03) // Distortion request
            {
                char *username = strtok(request.data, "&");
                char *filename = strtok(NULL, "&");
                char *factorStr = strtok(NULL, "&");

                if (username && filename && factorStr)
                {
                    int factor = atoi(factorStr);
                    char *extension = strrchr(filename, '.');

                    if (!extension)
                    {
                        printError("Invalid file extension. Aborting task.\n");
                        continue;
                    }

                    if (strcasecmp(extension, ".wav") == 0)
                    {
                        handleDistortionTask("audio", filename, factor);
                    }
                    else if (strcasecmp(extension, ".png") == 0 || strcasecmp(extension, ".jpg") == 0)
                    {
                        handleDistortionTask("image", filename, factor);
                    }
                    else
                    {
                        printError("Unsupported file type for distortion.\n");
                    }
                }
                else
                {
                    printError("Invalid request format from client.\n");
                }

                free(request.data);
                close(clientSocketFD);
            }
        }
    }

    close(listenFD);
    pthread_exit(NULL);
}

/**
 * @brief Handles failure recovery if the primary worker crashes.
 */
void handleFailureRecovery()
{
    pthread_mutex_lock(&primaryMutex);

    if (isPrimaryWorker)
    {
        printToConsole("Primary worker failed. Handling recovery...\n");
        // Implement logic to notify Gotham and reassign tasks
        // Notify connected Harleys to synchronize state
    }

    pthread_mutex_unlock(&primaryMutex);
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
    if (response.type != 0x02 || (response.dataLength && strcmp(response.data, "CON_KO") == 0))
    {
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
        closeProgramSignal();
    }

    pthread_t taskListenerThread;
    pthread_create(&taskListenerThread, NULL, listenForTasks, NULL);

    pthread_join(taskListenerThread, NULL);
    closeProgramSignal();
    return 0;
}