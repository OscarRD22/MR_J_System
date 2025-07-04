#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils/utils_connect.h"
#include "utils/utils_file.h"
#include "struct_definitions.h"
#include "utils/io_utils.h"
#include "so_compression.h"
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

// Osar.romero - Marc.marza

// This is the client
Harley_enigma harley;
int gothamSocketFD, listenFleckFD, clientFleckFD;
pthread_t DistorsionFleckThread;
fd_set master_set;
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
        SocketMessage message = {
            .type = 0x07, // Respuesta exitosa
            .dataLength = strlen(harley.worker_type),
            .data = strdup(harley.worker_type),
        };
        sendSocketMessage(gothamSocketFD, message);
        close(gothamSocketFD);
    }

    if (listenFleckFD > 0)
    {
        close(listenFleckFD);
    }

    if(clientFleckFD > 0)
    {
        SocketMessage message = {
            .type = 0x07, // Respuesta exitosa
            .dataLength = strlen(harley.worker_type),
            .data = strdup(harley.worker_type),
        };
        sendSocketMessage(clientFleckFD, message);
        close(clientFleckFD);
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

    close(data_file_fd);
}

/**
 * @brief Closes the program correctly cleaning the memory and closing the file descriptors
 */
void closeProgramSignal()
{
    printToConsole("\nClosing program Harley\n");

    closeFds();
    freeMemory();
    close(0); //Teclado
    close(1); //Pantalla
    close(2); //Error
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
    if (asprintf(&messageBuffer, "%s&%s&%d", harley.worker_type, harley.fleck_ip, harley.fleck_port) < 0)
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

/*void simulateDistortionProcess(int clientSocketFD)
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
    // releaseWorker(harley.gotham_ip, harley.gotham_port);
}*/

/**
 * @brief Distorts the file
 * @param path The path of the file
 * @param factor The factor of distortion
 */
void distortionFile(char *path, char *factor, char *filename)
{
    printToConsole("Distorsionando archivo\n");
    char *extension = strrchr(path, '.');
    extension = strrchr(factor, '2');
    extension = strrchr(filename, '.');

    if (strstr(extension, ".png") || strstr(extension, ".jpg"))
    {

        //int errorImg = 0;
        int errorImg = SO_compressImage(path, atoi(factor));

        if (errorImg != 0)
        {
            //printError("Error al distorsionar la imagen\n");
            return;
        }
    }
    else if (strstr(extension, ".wav"))
    {
        //int errorAudio = 0;
        int errorAudio = SO_compressAudio(path, atoi(factor));
        if (errorAudio != 0)
        {
            //printError("Error al distorsionar el audio\n");
            return;
        }
    }
    else
    {
        printError("Formato de archivo no soportado\n");
        return;
    }

    printToConsole("Archivo distorsionado correctamente\n");
}

/**
 * @brief Manages the distortion process
 * @param receivedMessage The message received from Fleck
 * @param fd_Fleck The file descriptor of the Fleck socket
 */
void managerDistorcion(SocketMessage receivedMessage, int fd_Fleck)
{
    // § DATA: <userName>&<FileName>&<FileSize>&<MD5SUM>&<factor>*/

    char *username = strtok(receivedMessage.data, "&");
    char *filename = strtok(NULL, "&");
    char *filesize = strtok(NULL, "&");
    char *md5sum = strtok(NULL, "&");
    char *factor = strtok(NULL, "&");

    char message[256];
    snprintf(message, sizeof(message), "Username:%s - Filename:%s - Filesize:%s - Md5sum:%s - Factor:%s \n", username, filename, filesize, md5sum, factor);
    printToConsole(message);

    if (strlen(username) == 0 || strlen(filename) == 0 || strlen(filesize) == 0 || strlen(md5sum) == 0 || strlen(factor) == 0)
    {
        printError("Error al leer el mensaje\n");
        SocketMessage messageToSend = {
            .type = 0x03,
            .dataLength = strlen("CON_KO"),
            .data = "CON_KO"};
        sendSocketMessage(fd_Fleck, messageToSend);
        return;
    }

    // Crear el mensaje a enviar a Gotham
    char *messageBuffer = NULL;
    if (asprintf(&messageBuffer, "%s&%s&%s&%s&%s", username, filename, filesize, md5sum, factor) < 0)
    {
        printError("Error al asignar memoria para el mensaje");
        return;
    }

    SocketMessage messageToSend = {
        .type = 0x03,
        .dataLength = 0,
        .data = NULL,
    };

    free(messageBuffer);

    sendSocketMessage(fd_Fleck, messageToSend);
    free(messageToSend.data);

    char *path = NULL;
    asprintf(&path, "%s/%s", harley.folder, filename);

    receiveFile(fd_Fleck, path);
    int result = compareMD5Sum(path, md5sum);

    if (result != 0)
    {
        printError("Error al recibir el archivo\n");
        SocketMessage messageToErrorSend = {
            .type = 0x06,
            .dataLength = strlen("CHECK_KO"),
            .data = "CHECK_KO"};
        sendSocketMessage(fd_Fleck, messageToErrorSend);
        return;
    }

    SocketMessage messageToSendOk = {
        .type = 0x06,
        .dataLength = strlen("CHECK_OK"),
        .data = "CHECK_OK"};
    sendSocketMessage(fd_Fleck, messageToSendOk);

    // Proceso de distorsión
    distortionFile(path, factor, filename);

    char *dataInfo = NULL;
    asprintf(&dataInfo, "%d&%s", getFileSize(path), md5sum);

    SocketMessage messageToSendDistorted = {
        .type = 0x04,
        .dataLength = strlen(dataInfo),
        .data = dataInfo,
    };
    sendSocketMessage(fd_Fleck, messageToSendDistorted);

    // Enviar archivo distorsionado
    sendFile(fd_Fleck, path);
    printToConsole("Enviado archivo distorsionado - OK\n");
    free(path);

    SocketMessage receivedMSG = getSocketMessage(fd_Fleck);
    
    char *printText = NULL;
    asprintf(&printText, "Mensaje recibido: Type: %d, Data: %s\n", receivedMSG.type, receivedMSG.data);
    printToConsole(printText);
    free(printText);

    if (receivedMSG.type == 0x07)
    {
        free(receivedMSG.data);
        close(fd_Fleck);
        clientFleckFD = -1;
        FD_CLR(fd_Fleck, &master_set);
        printToConsole("Conexión cerrada.\n");
        return;
    }

    return; // Éxito
}

void *listenToFlexDistorts()
{
    // Crear socket de escucha
    listenFleckFD = createAndListenSocket(harley.fleck_ip, harley.fleck_port);
    if (listenFleckFD < 0)
    {
        printToConsole("Error creating Fleck socket\n");
        pthread_exit(NULL);
    }
    printToConsole("Waiting for FLECK connections...............\n\n");

    // Inicializar conjuntos de descriptores de archivo
    fd_set read_set;
    FD_ZERO(&master_set);
    FD_ZERO(&read_set);

    // Agregar el socket de escucha al conjunto maestro
    FD_SET(listenFleckFD, &master_set);
    FD_SET(gothamSocketFD, &master_set); // Esperar actividad en los descriptores de archivo

    int max_fd = listenFleckFD; // Máximo descriptor de archivo

    while (1)
    {

        read_set = master_set; // Copiar el conjunto maestro para select()
        int activity = select(max_fd + 1, &read_set, NULL, NULL, NULL);

        if (activity < 0)
        {
            printError("Error en select()\n");
            break;
        }

        // Iterar sobre los descriptores de archivo activos
        for (int fd = 0; fd <= max_fd; ++fd)
        {
            if (FD_ISSET(fd, &read_set))
            {
                if (fd == listenFleckFD)
                {

                    // Nueva conexión entrante
                    int newSocketFD = accept(listenFleckFD, NULL, NULL);
                    if (newSocketFD < 0)
                    {
                        printError("Error al aceptar conexión de Fleck\n");
                        continue;
                    }

                    // Agregar el nuevo socket al conjunto maestro
                    FD_SET(newSocketFD, &master_set);
                    if (newSocketFD > max_fd)
                    {
                        max_fd = newSocketFD; // Actualizar el máximo descriptor de archivo
                    }
                    printToConsole("Nueva conexión de Fleck aceptada.\n");
                }
                else
                {

                    printToConsole("Nuevo mensaje recibido\n");
                    // Socket existente - recibir mensaje
                    SocketMessage receivedMessage = getSocketMessage(fd);
                    char *printText = NULL;
                    asprintf(&printText, "Mensaje recibido: Type: %d, Data: %s\n", receivedMessage.type, receivedMessage.data);
                    printToConsole(printText);
                    free(printText);


                    // Manejar el tipo de mensaje
                    switch (receivedMessage.type)
                    {
                    case 0x03:
                        /*Trama  per  demanar  una  connexió  al  Worker  de  media. Aquesta  ja  inclou  información
                        del fitxer a distorsionar.*/
                        clientFleckFD = fd;
                        managerDistorcion(receivedMessage, fd);

                        break;

                    case 0x04:
                        /*Trama per a enviar un fitxer de Fleck a Worker o quan el Worker contesta amb el fitxer
                        ja distorsionat, de Worker a Fleck.
                        Al  connectar-se,  ja  s’envia  la  informació  necessària  sobre  la  mida  del  fitxer,  factor  de
                        compressió o md5sum al origen.
                        Quan  el  Worker  contesti  amb  la  imatge  distorsionada  a  Fleck,  sí  caldrà  enviar  una
                        primera trama amb la informació del fitxer:
                        § TYPE: 0x04
                        § DATA_LENGTH: Llargària de les dades.
                        § DATA: <FileSize>&<MD5SUM>
                        No envía el nom del fitxer donat que aquest es queda igual. */

                    case 0x05:
                        /*Posteriorment, s’enviarà les dades del fitxer.
                        § TYPE: 0x05
                        § DATA_LENGTH: Llargària de les dades.
                        § DATA: dades_del_fitxer
                        Cal tenir en compte que caldrà enviar N trames de 256 bytes per a cada fitxer.*/
                        break;

                    case 0x07:
                        // Trama per a tancar la connexió entre Fleck i Worker.
                        // close(fd);
                        // FD_CLR(fd, &master_set);
                        // printToConsole("Conexión cerrada.\n");
                        // break;

                    default: // Tipo no reconocido
                        printToConsole("Tipo de mensaje no reconocido. Enviando trama de error...\n");
                        {
                            SocketMessage errorResponse = {0x09, 0, NULL, (unsigned int)time(NULL), 0};
                            sendSocketMessage(fd, errorResponse);
                        }
                        break;
                    }

                    // Liberar recursos del mensaje recibido
                    free(receivedMessage.data);
                }
            }
        }
    }

    close(listenFleckFD);
    printToConsole("CLOSING FLECK SOCKET\n");
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    initalSetup(argc);
    printToConsole("Reading configuration file.\n");
    saveHarley(argv[1]);
    printToConsole("Connecting Harley worker to the system...");
    int result = -1;

    while (result != 0)
    {
        printToConsole("Connected to Mr. J System, ready to listen to Fleck petitions\n");
        printToConsole("\nWaiting for connections...\n");

        result = connectHarleyToGotham();
        
        char *printText = NULL;
        asprintf(&printText, "Result: %d\n", result);
        printToConsole(printText);
        
        free(printText);
        // Intentar conexión a Gotham
        if (result != 0)
        {
            printError("Failed to connect to Gotham. Retrying in 2 seconds...\n");
            sleep(2);
            // continue;
        }
    }

    if (pthread_create(&DistorsionFleckThread, NULL, (void *)listenToFlexDistorts, NULL) != 0)
    {
        printError("Error creating Enigma thread\n");
        exit(1);
    }

    pthread_join(DistorsionFleckThread, NULL);

    // listenToFlexDistorts();
    //  printToConsole("Connected to Mr. J System, ready to listen to Fleck petitions\n");

    // Mantener la conexión y procesar mensajes

    /*while (1)
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
*/
    closeProgramSignal();
    return 0;
}
