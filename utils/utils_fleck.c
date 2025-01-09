#include "io_utils.h"
#include "utils_fleck.h"
#include "stdio.h"
#include "utils_connect.h"
#include <time.h>
#include "sys/stat.h"
#include "../struct_definitions.h"

#define MAX_FILES 100

// Osar.romero - Marc.marza
extern Fleck fleck;
int gothamSocketFD, distorsionSocketFD;
int isDistorsionConnected = FALSE;
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

//toDo----------------------------------------------------- CONNECT ------------------------------------------------------------------

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
//toDo----------------------------------------------------- CONNECT ------------------------------------------------------------------

//toDo----------------------------------------------------- DISTORT ------------------------------------------------------------------

/**
 * @brief Conecta a Harley con los datos recibidos de Gotham (IP, puerto) y se desconecta al finalizar.
 * @param workerIP La IP del worker (Harley) proporcionada por Gotham.
 * @param workerPort El puerto del worker (Harley) proporcionado por Gotham.
 */
void connectToHarleyAndDisconnect(const char *workerIP, int workerPort)
{
    // Crear y conectar el socket al worker Harley
    int workerSocketFD = createAndConnectSocket(workerIP, workerPort, FALSE);
    if (workerSocketFD < 0)
    {
        printError("Failed to connect to Worker (Harley).\n");
        return;
    }

    printToConsole("Connected to Harley worker successfully.\n");

    // Simular operación de distorsión
    // Puedes enviar mensajes simples para demostrar la interacción
    SocketMessage initialMessage = {
        .type = 0x03, // Tipo de mensaje para iniciar conexión con Harley
        .dataLength = 0,
        .data = NULL,
        .timestamp = (unsigned int)time(NULL),
        .checksum = 0};

    sendSocketMessage(workerSocketFD, initialMessage);

    // Esperar respuesta de Harley
    SocketMessage response = getSocketMessage(workerSocketFD);
    if (response.type == 0x03 && response.dataLength == 0)
    {
        printToConsole("Harley confirmed connection and ready to operate.\n");
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
        return;
    }

    free(response.data);

    // Notificar desconexión a Harley
    SocketMessage disconnectMessage = {
        .type = 0x07, // Tipo de mensaje para desconexión
        .dataLength = strlen(fleck.username),
        .data = strdup(fleck.username), // Enviamos el username como identificador
        .timestamp = (unsigned int)time(NULL),
        .checksum = calculateChecksum(fleck.username, strlen(fleck.username))};

    sendSocketMessage(workerSocketFD, disconnectMessage);
    free(disconnectMessage.data);

    printToConsole("Disconnected from Harley worker successfully.\n");

    // Cerrar el socket
    close(workerSocketFD);
}

/**
 * @brief Envia una petició de distorsió a Gotham i processa la seva resposta.
 * @param mediaType El tipus de media (extensió del fitxer, com "png", "wav").
 * @param fileName El nom del fitxer a distorsionar.
 * @return 0 si Gotham retorna un worker disponible, -1 en cas d'error o "DISTORT_KO".
 */
int sendDistortRequestToGotham(const char *mediaType, const char *fileName)
{

    // Crear y conectar el socket
    if ((gothamSocketFD = createAndConnectSocket(fleck.ip, fleck.port, FALSE)) < 0)
    {
        printError("Error connecting to Gotham\n");
        return -1;
    }

    // Construir el missatge per enviar a Gotham
    char *dataBuffer = NULL;
    if (asprintf(&dataBuffer, "%s&%s", mediaType, fileName) < 0)
    {
        printError("Failed to allocate memory for distortion request.\n");
        return -1;
    }

    SocketMessage request;
    request.type = 0x10; // Tipus de petició de distorsió
    request.dataLength = strlen(dataBuffer);
    request.data = dataBuffer;
    request.timestamp = (unsigned int)time(NULL);
    request.checksum = calculateChecksum(dataBuffer, request.dataLength);

    printToConsole("Enviando solicitud de distorsión a Gotham...");

    printf("Datos de la Solicitud Data: %s\n", request.data);

    sendSocketMessage(gothamSocketFD, request);
    free(dataBuffer);
    printf("\nMensaje a Gotham enviado\n");

    // Esperar resposta de Gotham
    SocketMessage response = getSocketMessage(gothamSocketFD);
    printf("Respuesta de Gotham: Type: %d, Data: %s\n", response.type, response.data);
    //! Creo que Gotham si envia bien ip y puerto no se porque falla aqui al recibir el mensaje

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
    connectToHarleyAndDisconnect(workerIP, workerPort);

    free(response.data);
    return 0; // Èxit
}

/**
 * @brief Clears all resources and disconnects from Gotham.
 */
void handleDistortCommand(const char *fileName, const char *factor)
{
    char *extension = strrchr(fileName, '.');
    if (!extension || strlen(extension) < 2)
    {
        printError("Invalid file type. Please provide a valid file.\n");
        return;
    }

    // Treure l'extensió (sense el punt inicial)
    char *mediaType = extension + 1;
    printf("Estoy dentro de handleDistortCommand - MediaType: %s\n", mediaType);

    
    // Enviar petició a Gotham
    if (sendDistortRequestToGotham(mediaType, fileName) == 0)
    {
        printToConsole("Distortion process completed successfully.\n");
    }
    else
    {
        printError("Failed to process distortion request.\n");
    }
}

//toDo----------------------------------------------------- DISTORT ------------------------------------------------------------------
