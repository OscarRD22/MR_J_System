#include "io_utils.h"
#include "utils_fleck.h"
#include "stdio.h"
#include "utils_connect.h"
#include <time.h>
#include "../struct_definitions.h"

// Osar.romero - Marc.marza

extern Fleck fleck;

int gothamSocketFD, distorsionSocketFD;
int isDistorsionConnected = FALSE;
#define MAX_FILES 100
int DISTORSION = FALSE;         // Indica si hay un proceso de distorsión en curso
int finishedDistortion = FALSE; // Indica si la última distorsión terminó con éxito

void listMedia()
{
    DIR *dir;
    struct dirent *ent;
    char *fileNames[MAX_FILES];
    int count = 0;

    dir = opendir("files");
    if (dir == NULL)
    {
        printError("Error opening directory\n");
        return;
    }

    // Recorrido: almacenar los nombres de archivos de medios en el array
    while ((ent = readdir(dir)) != NULL)
    {
        // Filtrar las entradas "." y ".." y excluir archivos ".txt"
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0)
        {
            if (!strstr(ent->d_name, ".txt"))
            { // Excluir archivos de texto
                if (count < MAX_FILES)
                {                                           // Verificar el límite
                    fileNames[count] = strdup(ent->d_name); // Almacenar el nombre
                    count++;
                }
            }
        }
    }
    closedir(dir);

    // Mostrar el encabezado si hay archivos de medios
    if (count > 0)
    {
        char *buffer = NULL;
        asprintf(&buffer, "There are %d media files available:\n", count);
        printToConsole(buffer); // Imprimir el encabezado
        free(buffer);           // Liberar la memoria

        // Imprimir la lista de archivos
        for (int i = 0; i < count; i++)
        {
            asprintf(&buffer, "%d. %s\n", i + 1, fileNames[i]);
            printToConsole(buffer);
            free(buffer);       // Liberar la memoria de cada línea de impresión
            free(fileNames[i]); // Liberar memoria de cada nombre de archivo
        }
    }
    else
    {
        printToConsole("No media files found\n");
    }
}

void listText()
{
    DIR *dir;
    struct dirent *ent;
    char *fileNames[MAX_FILES];
    int count = 0;

    dir = opendir("files");
    if (dir == NULL)
    {
        printError("Error opening directory\n");
        return;
    }

    // Recorrido: almacenar los nombres de archivos de texto en el array
    while ((ent = readdir(dir)) != NULL)
    {
        // Filtrar las entradas "." y ".." y excluir archivos no ".txt"
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0)
        {
            if (strstr(ent->d_name, ".txt"))
            { // Incluir solo archivos .txt
                if (count < MAX_FILES)
                {                                           // Verificar el límite
                    fileNames[count] = strdup(ent->d_name); // Almacenar el nombre
                    count++;
                }
            }
        }
    }
    closedir(dir);

    // Mostrar el encabezado si hay archivos de texto
    if (count > 0)
    {
        char *buffer = NULL;
        asprintf(&buffer, "There are %d text files available:\n", count);
        printToConsole(buffer); // Imprimir el encabezado
        free(buffer);           // Liberar la memoria

        // Imprimir la lista de archivos de texto
        for (int i = 0; i < count; i++)
        {
            asprintf(&buffer, "%d. %s\n", i + 1, fileNames[i]);
            printToConsole(buffer);
            free(buffer);       // Liberar la memoria de cada línea de impresión
            free(fileNames[i]); // Liberar memoria de cada nombre de archivo
        }
    }
    else
    {
        printToConsole("No text files found\n");
    }
}
/**
 * @brief Connects to the Gotham server with stable connection.
 * @param isExit Indicates whether the Fleck is exiting (send disconnect to Gotham) or not (send connection request).
 * @return int Returns 0 on success, -1 on failure.
 */
int connectToGotham(int isExit)
{
    // Crear y conectar el socket
    if ((gothamSocketFD = createAndConnectSocket(fleck.ip, fleck.port, FALSE)) < 0)
    {
        printError("Error connecting to Gotham\n");
        return -1;
    }

    // Preparar el mensaje para enviar a Gotham
    SocketMessage message;
    char *buffer = NULL;

    if (!isExit)
    {
        // Solicitud de conexión
        if (asprintf(&buffer, "%s&%s&%d", fleck.username, fleck.ip, fleck.port) < 0)
        {
            printError("Error allocating memory for connection message\n");
            close(gothamSocketFD);
            return -1;
        }
        message.type = 0x01; // Tipo de mensaje de conexión
    }
    else
    {
        // Solicitud de desconexión
        if (asprintf(&buffer, "%s", fleck.username) < 0)
        {
            printError("Error allocating memory for disconnection message\n");
            close(gothamSocketFD);
            return -1;
        }
        message.type = 0x07; // Tipo de mensaje de desconexión
    }

    message.dataLength = strlen(buffer);
    message.data = strdup(buffer);
    message.timestamp = (unsigned int)time(NULL);
    message.checksum = calculateChecksum(message.data, message.dataLength);

    printf("Message being sent: %s\n", buffer);

    // Enviar el mensaje a Gotham
    sendSocketMessage(gothamSocketFD, message);
    printf("Type: %d, DataLength: %d, Data: %s, Timestamp: %u, Checksum: %d\n",
           message.type, message.dataLength, message.data, message.timestamp, message.checksum);

    // Liberar el buffer del mensaje
    free(buffer);
    free(message.data);

    // Recibir la respuesta de Gotham
    SocketMessage response = getSocketMessage(gothamSocketFD);

    // Manejar la respuesta
    if (!isExit)
    {
        // Respuesta a la solicitud de conexión
        if (response.type == 0x01)
        {
            printf("Connected to Gotham successfully!\n");
        }
        else
        {
            printError("Error connecting to Gotham: Invalid response type\n");
            free(response.data);
            close(gothamSocketFD);
            return -1;
        }
    }
    else
    {
        // Respuesta a la solicitud de desconexión
        if (response.type == 0x07 && strcmp(response.data, "CON_KO") == 0)
        {
            printError("Error disconnecting from Gotham\n");
            free(response.data);
            close(gothamSocketFD);
            return -1;
        }
        printf("Disconnected from Gotham successfully!\n");
    }

    // Liberar recursos y cerrar el socket
    free(response.data);
    
    return 0;
}

/**
 * @brief If the Fleck is connected to the Enigma/Harlay server, it disconnects from it and sends a message to the Gotham server
 */
void logout()
{
    // pthread_mutex_lock(&isConnectedMu);
    if (isDistorsionConnected == TRUE)
    {
        SocketMessage m;
        m.type = 0x07;
        m.dataLength = strlen(fleck.username);
        m.data = strdup(fleck.username);

        sendSocketMessage(distorsionSocketFD, m);

        free(m.data);

        sleep(1);
        isDistorsionConnected = FALSE;
        // pthread_mutex_unlock(&isConnectedMu);

        // pthread_join(listenThread, NULL);
        // printToConsole("Thread joined\n");
        close(distorsionSocketFD);
        return;
    }
    // pthread_mutex_unlock(&isConnectedMu);
}

/**
 * @brief Envía un archivo al Worker en tramas de 256 bytes.
 * @param workerSocketFD Socket del Worker.
 * @param filename Nombre del archivo a enviar.
 */
void sendFileToWorker(int workerSocketFD, const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        printError("Failed to open file for reading.\n");
        return;
    }

    char buffer[256];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        SocketMessage fileMessage;
        fileMessage.type = 0x05;
        fileMessage.dataLength = bytesRead;
        fileMessage.data = malloc(bytesRead);
        memcpy(fileMessage.data, buffer, bytesRead);
        fileMessage.timestamp = (unsigned int)time(NULL);
        fileMessage.checksum = calculateChecksum(fileMessage.data, fileMessage.dataLength);

        sendSocketMessage(workerSocketFD, fileMessage);

        free(fileMessage.data);
    }

    fclose(file);
}

/**
 * @brief Maneja el comando RESUME para continuar la distorsión tras un fallo.
 * @param filename Nombre del archivo a distorsionar.

void handleResumeCommand(const char *filename)
{
    char *extension = strrchr(filename, '.');
    if (!extension || strlen(extension) < 2)
    {
        printError("Invalid file type. Please provide a valid file.\n");
        return;
    }

    // Construir la trama para Gotham
    char *dataBuffer = NULL;
    asprintf(&dataBuffer, "%s&%s", extension + 1, filename);

    SocketMessage resumeRequest;
    resumeRequest.type = 0x11;
    resumeRequest.dataLength = strlen(dataBuffer);
    resumeRequest.data = strdup(dataBuffer);
    resumeRequest.timestamp = (unsigned int)time(NULL);
    resumeRequest.checksum = calculateChecksum(resumeRequest.data, resumeRequest.dataLength);

    free(dataBuffer);

    // Enviar solicitud de reanudación a Gotham
    sendSocketMessage(gothamSocketFD, resumeRequest);
    free(resumeRequest.data);

    // Recibir respuesta de Gotham
    SocketMessage response = getSocketMessage(gothamSocketFD);

    if (response.type == 0x11)
    {
        if (strcmp(response.data, "DISTORT_KO") == 0 || strcmp(response.data, "MEDIA_KO") == 0)
        {
            printError("Gotham could not assign a new Worker. Distortion failed.\n");
        }
        else
        {
            char *workerIP = strtok(response.data, "&");
            char *workerPortStr = strtok(NULL, "&");

            if (workerIP && workerPortStr)
            {
                int workerPort = atoi(workerPortStr);
                printToConsole("Resuming distortion with a new Worker...\n");

                int workerSocketFD = createAndConnectSocket(workerIP, workerPort, FALSE);
                if (workerSocketFD < 0)
                {
                    printError("Failed to connect to new Worker.\n");
                }
                else
                {
                    sendFileToWorker(workerSocketFD, filename);
                    close(workerSocketFD);
                }
            }
            else
            {
                printError("Invalid response format from Gotham.\n");
            }
        }
    }
    else
    {
        printError("Invalid response type from Gotham.\n");
    }

    free(response.data);
}
 */


/**
 * @brief Maneja el comando DISTORT enviando la solicitud a Gotham y conectándose al Worker.
 * @param filename Nombre del archivo a distorsionar.
 * @param factor Factor de distorsión.
 */
void handleDistortCommand(const char *filename, const char *factor)
{
    DISTORSION = TRUE; // Iniciar proceso de distorsión

    char *extension = strrchr(filename, '.');
    if (!extension || strlen(extension) < 2)
    {
        printError("Invalid file type. Please provide a valid file.\n");
        return;
    }

    // Construir la trama para Gotham
    char *dataBuffer = NULL;
    asprintf(&dataBuffer, "%s&%s", extension + 1, filename);

    SocketMessage distortRequest;
    distortRequest.type = 0x10;
    distortRequest.dataLength = strlen(dataBuffer);
    distortRequest.data = strdup(dataBuffer);
    distortRequest.timestamp = (unsigned int)time(NULL);
    distortRequest.checksum = calculateChecksum(distortRequest.data, distortRequest.dataLength);

    // Enviar solicitud a Gotham
    sendSocketMessage(gothamSocketFD, distortRequest);
    free(dataBuffer);
    free(distortRequest.data);
    
    
    printToConsole("ESTOY AQUIIII..\n");

    // Recibir respuesta de Gotham
    SocketMessage response = getSocketMessage(gothamSocketFD);

    printToConsole("Gotham me ha respondido\n");

    if (response.type == 0x10)
    {
        if (strcmp(response.data, "DISTORT_KO") == 0)
        {
            printError("Gotham could not process the distortion request. No workers available.\n");
            DISTORSION = FALSE;
        }
        else
        {
            char *workerIP = strtok(response.data, "&");
            char *workerPortStr = strtok(NULL, "&");
            
            

            if (workerIP && workerPortStr)
            {
                int workerPort = atoi(workerPortStr);
                printToConsole("Connecting to Worker...\n");

                int workerSocketFD = createAndConnectSocket(workerIP, workerPort, FALSE);
                if (workerSocketFD < 0)
                {
                    printError("Failed to connect to Worker.\n");
                    DISTORSION = FALSE;
                }
                else
                {
                    // Enviar solicitud al Worker
                    char *workerRequestData = NULL;
                    asprintf(&workerRequestData, "%s&%s&%s", fleck.username, filename, factor);

                    SocketMessage workerRequest;
                    workerRequest.type = 0x03;
                    workerRequest.dataLength = strlen(workerRequestData);
                    workerRequest.data = strdup(workerRequestData);
                    workerRequest.timestamp = (unsigned int)time(NULL);
                    workerRequest.checksum = calculateChecksum(workerRequest.data, workerRequest.dataLength);

                    sendSocketMessage(workerSocketFD, workerRequest);
                    free(workerRequest.data);
                    free(workerRequestData);

                    // Recibir confirmación del Worker
                    SocketMessage workerResponse = getSocketMessage(workerSocketFD);

                    if (workerResponse.type == 0x03 && workerResponse.dataLength == 0)
                    {
                        printToConsole("Worker ready to distort the file.\n");

                        // Enviar el archivo al Worker
                        sendFileToWorker(workerSocketFD, filename);
                        DISTORSION = FALSE;
                        finishedDistortion = TRUE;
                    }
                    else
                    {
                        printError("Worker failed to start distortion.\n");
                        DISTORSION = FALSE;
                    }

                    free(workerResponse.data);
                    close(workerSocketFD);
                }
            }
            else
            {
                printError("Invalid response format from Gotham.\n");
                DISTORSION = FALSE;
            }
        }
    }
    else
    {
        printError("Invalid response type from Gotham.\n");
        DISTORSION = FALSE;
    }

    free(response.data);
}




