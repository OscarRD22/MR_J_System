#include "io_utils.h"
#include "utils_fleck.h"
#include "stdio.h"
#include "utils_connect.h"
#include <time.h>
#include "sys/stat.h"
#include "../struct_definitions.h"
#include "../utils/utils_file.h"

#define MAX_FILES 100

// Osar.romero - Marc.marza

extern Fleck fleck;
int gothamSocketFD, distorsionSocketFD;
int isDistorsionConnected = FALSE;
int DISTORSION = FALSE;         // Indica si hay un proceso de distorsión en curso
int finishedDistortion = FALSE; // Indica si la última distorsión terminó con éxito
int isTxtDistortRunning = FALSE;
int isMediaDistortRunning = FALSE;
pthread_mutex_t isTxtDistortRunningMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t isMediaDistortRunningMutex = PTHREAD_MUTEX_INITIALIZER;
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

// toDo----------------------------------------------------- CONNECT ------------------------------------------------------------------

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
    // printf("Type: %d, DataLength: %d, Data: %s, Timestamp: %u, Checksum: %d\n",
    //        message.type, message.dataLength, message.data, message.timestamp, message.checksum);

    // Liberar el buffer del mensaje
    free(buffer);
    free(message.data);

    // Recibir la respuesta de Gotham
    SocketMessage response = getSocketMessage(gothamSocketFD);

    // Manejar la respuesta
    if (!isExit && response.type == 0x01)
    {
        printToConsole("Connected to Gotham successfully!");
    }
    else if (isExit && response.type == 0x07 && response.data != NULL && strcmp(response.data, "CON_KO") != 0)
    {
        printToConsole("Disconnected from Gotham successfully!");
        close(gothamSocketFD);
    }
    else
    {
        printError("Unexpected response from Gotham");
        free(response.data);
        close(gothamSocketFD);
        return -1;
    }

    free(response.data);
    // close(gothamSocketFD);
    return 0;
}
// toDo----------------------------------------------------- CONNECT ------------------------------------------------------------------

// toDo----------------------------------------------------- DISTORT ------------------------------------------------------------------

/**
 * @brief Conecta al WORKER con los datos recibidos de Gotham (IP, puerto) y se desconecta al finalizar.
 * @param workerIP La IP del worker (Harley) proporcionada por Gotham.
 * @param workerPort El puerto del worker (Harley) proporcionado por Gotham.
 */
int connectToWorkerAndDisconnect(char *workerIP, int workerPort, char *fullPath, char *fileName, char *factor)
{
    // Crear y conectar el socket al worker Harley
    int workerSocketFD = createAndConnectSocket(workerIP, workerPort, FALSE);
    if (workerSocketFD < 0)
    {
        printError("Failed to connect to Worker (Harley).\n");
        return -1;
    }

    printToConsole("Connected to Harley worker successfully.\n");

    int filesize = getFileSize(fullPath);

    char *md5sum = calculateMD5(fullPath);

    //<userName>&<fullPath>&<FileSize>&<MD5SUM>&<factor>
    char *data = NULL;
    if (asprintf(&data, "%s&%s&%d&%s&%s", fleck.username, fileName, filesize, md5sum, factor) < 0)
    {
        printError("Failed to allocate memory for distortion request.\n");
        return -1;
    }

    // char *message = NULL;
    // asprintf(&message, "Username:%s - fullPath:%s - Filesize:%d - Md5sum:%s - Factor:%s \n", fleck.username, fullPath, filesize, md5sum, factor);
    // printToConsole(message);

    // Puedes enviar mensajes simples para demostrar la interacción
    SocketMessage initialMessage = {
        .type = 0x03, // Tipo de mensaje para iniciar conexión con Harley
        .dataLength = strlen(data),
        .data = data};

    sendSocketMessage(workerSocketFD, initialMessage);

    // Esperar respuesta de Harley
    SocketMessage response = getSocketMessage(workerSocketFD);
    printf("Respuesta de Harley: Type: %d, Data: %s\n", response.type, response.data);
    if (response.type == 0x03 && response.dataLength == 0)
    {
        printToConsole("Harley confirmed connection and ready to operate.\n");

        // Enviar el archivo a Harley
        sendFile(workerSocketFD, fullPath);

        SocketMessage response = getSocketMessage(workerSocketFD); // Esperar confirmación de recepción
        if (response.type == 0x06 && response.data != NULL && strcmp(response.data, "CHECK_OK") == 0)
        {
            printToConsole("File received by Harley successfully.\n");

            SocketMessage response = getSocketMessage(workerSocketFD); // Esperar confirmación de distorsión FileSize y MD5SUM

            if (response.type == 0x04 && response.data != NULL)
            {
                char *fileSizeWorker = strtok(response.data, "&");
                char *md5sumWorker = strtok(NULL, "&");

                if (fileSizeWorker == NULL || md5sumWorker == NULL)
                {
                    printError("Failed to parse file size and MD5SUM from Harley.\n");
                    free(response.data);
                    return -1;
                }

                printf("Respuesta de Harley: Type: %d, Data: %s\n", response.type, response.data);

                char *allFileName = NULL;
                asprintf(&allFileName, "files_distorted/%s", fileName); // Nombre del archivo distorsionado

                receiveFile(workerSocketFD, allFileName);
                free(allFileName);
            }
            else if (response.type == 0x07 && response.data != NULL)
            {

                close(workerSocketFD);
                printToConsole("Worker disconnected while distort\n");
                free(response.data);
                return -2;
            }
            else
            {
                printError("Failed to send file to Harley.\n");
                free(response.data);
            }
        }
        printf("Archivo enviado a Harley\n");
    }
    else
    {
        printError("Worker disconnected unexpectedly. Notifying Gotham...\n");

        // Notificar a Gotham
        SocketMessage resumeRequest = {
            .type = 0x11,
            .dataLength = strlen("RESUME_REQUEST"),
            .data = strdup("RESUME_REQUEST"),
            .timestamp = (unsigned int)time(NULL),
            .checksum = calculateChecksum("RESUME_REQUEST", strlen("RESUME_REQUEST"))};

        sendSocketMessage(gothamSocketFD, resumeRequest);
        free(resumeRequest.data);
        close(workerSocketFD);
        return -1;
    }

    free(response.data);

    SocketMessage message = {
        .type = 0x07, // Respuesta exitosa
        .dataLength = strlen(fleck.username),
        .data = strdup(fleck.username),
    };
    sendSocketMessage(workerSocketFD, message); // gothamSocketFD

    free(message.data);

    printToConsole("Disconnected from Harley worker successfully.\n");

    // Cerrar el socket
    close(workerSocketFD);

    return 0; // Éxito
}

/**
 * @brief Envia una petició de distorsió a Gotham i processa la seva resposta.
 * @param mediaType El tipus de media (extensió del fitxer, com "png", "wav").
 * @param fileName El nom del fitxer a distorsionar.
 * @return 0 si Gotham retorna un worker disponible, -1 en cas d'error o "DISTORT_KO".
 */
int sendDistortRequestToGotham(char *mediaType, char *fullPath, char *filename, char *factor)
{

    // Crear y conectar el socket
    if ((gothamSocketFD = createAndConnectSocket(fleck.ip, fleck.port, FALSE)) < 0)
    {
        printError("Error connecting to Gotham\n");
        return -1;
    }

    // Construir el missatge per enviar a Gotham
    char *dataBuffer = NULL;
    if (asprintf(&dataBuffer, "%s&%s", mediaType, fullPath) < 0)
    {
        printError("Failed to allocate memory for distortion request.\n");
        return -1;
    }

    SocketMessage request;
    request.type = 0x10; // Tipus de petició de distorsió
    request.dataLength = strlen(dataBuffer);
    request.data = dataBuffer;

    printToConsole("Enviando solicitud de distorsión a Gotham...");

    printf("Datos de la Solicitud Data: %s\n", request.data);

    sendSocketMessage(gothamSocketFD, request);
    free(dataBuffer);
    printf("\nMensaje a Gotham enviado\n");

    // Esperar resposta de Gotham
    SocketMessage response = getSocketMessage(gothamSocketFD);
    printf("Respuesta de Gotham: Type: %d, Data: %s\n", response.type, response.data);

    if (response.type != 0x10)
    {
        printError("Invalid response type from Gotham.\n");
        free(response.data);
        return -1;
    }

    // Processar la resposta
    if (strcmp(response.data, "DISTORT_KO") == 0)
    {
        printError("No workers available or invalid media type.\n");
        free(response.data);
        return -1;
    }

    // Obtenir IP i port del worker des de la resposta
    char *workerIP = strtok(response.data, "&");
    char *workerPortStr = strtok(NULL, "&");

    if (!workerIP || !workerPortStr)
    {
        printError("Malformed response from Gotham.\n");
        free(response.data);
        return -1;
    }

    int workerPort = atoi(workerPortStr);

    printf("Worker Details: IP: %s, Port: %d\n", workerIP, workerPort);
    int inError = connectToWorkerAndDisconnect(workerIP, workerPort, fullPath, filename, factor);

    free(response.data);
    return inError;
}

/**
 * @brief Clears all resources and disconnects from Gotham.
 */
void handleDistortCommand(void *params)
{
    ThreadDistortionParams *distortionParams = (ThreadDistortionParams *)params;
    char *fullPath = distortionParams->fullPath;
    char *filename = distortionParams->filename;
    char *factor = distortionParams->factor;

    printf("PRINT: %s - %s - %s\n", fullPath, filename, factor);

    char *extension = strrchr(fullPath, '.');
    if (!extension || strlen(extension) < 2)
    {
        printError("Invalid file type. Please provide a valid file.\n");
        return;
    }

    // Treure l'extensió (sense el punt inicial)
    char *mediaType = extension + 1;

    // Enviar petició a Gotham
    /*
     if (sendDistortRequestToGotham(mediaType, fullPath, filename, factor) == 0)
      {


          printToConsole("Distortion process completed successfully.\n");
      }
      else
      {
          printError("Failed to process distortion request.\n");
      }
      */
    int error = 0;
    do
    {
        error = sendDistortRequestToGotham(mediaType, fullPath, filename, factor);
        if (error == -1)
        {
            printError("Unrecoverable error while distorting.\n");
        }
        else if (error == -2)
        {
            printError("Worker disconnected while distorting. Retraying....\n");
        }
        else
        {
            printToConsole("Distortion process completed successfully.\n");
        }
    } while (error == -2);

    // Alliberar memòria
    free(distortionParams->fullPath);
    free(distortionParams->filename);
    free(distortionParams->factor);
    free(distortionParams);
    if (strcasecmp(mediaType, "txt") == 0)
    {
        pthread_mutex_lock(&isTxtDistortRunningMutex);
        isTxtDistortRunning = FALSE;
        pthread_mutex_unlock(&isTxtDistortRunningMutex);
    }
    else
    {
        pthread_mutex_lock(&isMediaDistortRunningMutex);
        isMediaDistortRunning = FALSE;
        pthread_mutex_unlock(&isMediaDistortRunningMutex);
    }
}

// toDo----------------------------------------------------- DISTORT ------------------------------------------------------------------
