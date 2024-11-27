#include "io_utils.h"
#include "utils_fleck.h"
#include "utils_connect.h"
#include <time.h>
#include "../struct_definitions.h"

// Osar.romero - Marc.marza

extern Fleck fleck;

int gothamSocketFD, distorsionSocketFD;
int isDistorsionConnected = FALSE;
#define MAX_FILES 100

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
 * @brief Connects to the Gotham server with stable connection
 * @param isExit to know if the Fleck is exiting (need to send disconnect to Gotham) or not (need to connect to Harly/Enigma)
 */

int connectToGotham(int isExit)
{
    if ((gothamSocketFD = createAndConnectSocket(fleck.ip, fleck.port, FALSE)) < 0)
    {
        printError("Error connecting to Gotham\n");

        exit(1);
    }

    // CONNECTED TO GOTHAM
    SocketMessage m;
    if (isExit == FALSE)
    {
        char *buffer;
        asprintf(&buffer, "%s&%s&%d", fleck.username, fleck.ip, fleck.port);
        m.type = 0x01;
        m.dataLength = strlen(buffer);
        m.data = strdup(buffer);
        m.timestamp = (unsigned int)time(NULL);
        m.checksum = calculateChecksum(buffer, strlen(buffer));
        sendSocketMessage(gothamSocketFD, m);
        free(m.data);
    }
    else if (isExit == TRUE)
    {
        m.type = 0x07;
        char *buffer;
        asprintf(&buffer, "%s", fleck.username);
        m.dataLength = strlen(buffer);
        m.data = strdup(fleck.username);
        sendSocketMessage(gothamSocketFD, m);
        free(m.data);
    }

    // Receive response
    
     SocketMessage response = getSocketMessage(gothamSocketFD);

    // handle response
     if (isExit == FALSE)
     {
         if (response.type == 0x01 && strcmp(response.data, "CON_KO") == 0)
         {
             printError("Error connecting to Gotham NO SERVERS Harley/Enigma\n");
         }

         free(response.data);
         close(gothamSocketFD);

         return FALSE;
     }
     else if (isExit == TRUE)
     {
         if (response.type == 0x07 && strcmp(response.data, "CON_KO") == 0)
         {
             printError("Error disconnecting from Gotham\n");
             logout();
         }
         free(response.data);
         close(gothamSocketFD);

         return TRUE;
     }

     // It should never reach this point, but if it arrives, it returns -1.
     free(response.data);
     return -1;
    
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

void handleDistortCommand(char *filename, char *factor)
{

    int factorInt = atoi(factor);

    if (!filename || factorInt <= 0)
    {
        printError("Invalid DISTORT command: missing file name or factor\n");
        return;
    }

    char *extension = strrchr(filename, '.');
    if (!extension)
    {
        printError("Invalid file: missing extension\n");
        return;
    }

    char *workerType = NULL;
    if (strcmp(extension, ".txt") == 0)
    {
        // workerType = "Text";
        SocketMessage m;

        char *buffer;
        asprintf(&buffer, "%s&%s", workerType, filename);
        m.type = 0x10;
        m.dataLength = strlen(buffer);
        m.data = strdup(buffer);
        sendSocketMessage(gothamSocketFD, m);
        free(m.data);
        free(buffer);
  
    }
    else if (strcmp(extension, ".jpg") == 0 || strcmp(extension, ".png") == 0 ||
             strcmp(extension, ".wav") == 0)
    {
        workerType = "Media";
    }
    else
    {
        printError("Unsupported file extension\n");
        return;
    }
    // Esperando respuesta de Gotham para asignar worker a la distorsión.
    SocketMessage response = getSocketMessage(gothamSocketFD);

    if (response.type != 0x10 || strcmp(response.data, "DISTORT_KO") == 0) {
        printError("Gotham failed to assign a worker\n");
        close(gothamSocketFD);
        return;
    }

    asprintf("Assigned worker: %s\n", response.data);
    // Conectar al worker asignado
    char *workerIP = strtok(response.data, "&");
    char *workerPortStr = strtok(NULL, "&");
    int workerPort = atoi(workerPortStr);
/*
    //! ABRIR THREAD -----------------------------------------------------------------------

    int workerSocket = createAndConnectSocket(workerIP, workerPort, FALSE);
    if (workerSocket < 0) {
        printError("Failed to connect to assigned worker\n");
        close(gothamSocketFD);
        return;
    }

    //char *buffer;
    //asprintf(&buffer, "DISTORT  FILENAME(%s) FACTOR(%d) WORKER TYPE(%s)\n", filename, factorInt, workerType);
    //printToConsole(buffer);
    //free(buffer);

    //! F3 -----------------------------------------------------------------------
    //sendFileToWorker(workerSocket, filename, factor);

     Cerrar conexiones
    close(workerSocket);
    close(gothamSocketFD);
*/
    
}


/*
void sendFileToWorker(int workerSocket, char *filename, int factor) {
    // Leer el archivo en memoria
    int fileFD = open(filename, O_RDONLY);
    if (fileFD < 0) {
        printError("Error opening file for reading\n");
        return;
    }

    // Obtener el tamaño del archivo
    struct stat fileStat;
    fstat(fileFD, &fileStat);
    size_t fileSize = fileStat.st_size;

    // Calcular el checksum del archivo original
    char fileChecksum[33];
    calculateMD5Checksum(fileFD, fileChecksum); // Implementar esta función

    // Crear la trama inicial
    SocketMessage fileInfo;
    char *buffer;
    asprintf(&buffer, "%s&%lu&%s&%d", filename, fileSize, fileChecksum, factor);
    fileInfo.type = 0x03;
    fileInfo.dataLength = strlen(buffer);
    fileInfo.data = buffer;

    sendSocketMessage(workerSocket, fileInfo);
    free(buffer);

    // Enviar el archivo en tramas de 256 bytes
    char dataBuffer[256];
    ssize_t bytesRead;
    while ((bytesRead = read(fileFD, dataBuffer, 256)) > 0) {
        write(workerSocket, dataBuffer, bytesRead);
    }
    close(fileFD);

    // Recibir el archivo distorsionado
    SocketMessage response = getSocketMessage(workerSocket);
    if (response.type != 0x06 || strcmp(response.data, "CHECK_KO") == 0) {
        printError("Failed to process file distortion\n");
        return;
    }

    printf("File distortion successful: %s\n", response.data);
}

*/

