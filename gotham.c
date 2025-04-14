#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "struct_definitions.h"
#include "utils/io_utils.h"
#include "utils/utils_connect.h"

#define MAX_MESSAGE_SIZE 256
#define MAX_WORKERS 16
// Osar.romero - Marc.marza
// Estructura para un worker
typedef struct
{
    char workerType[10]; // Tipo: "Harley" o "Enigma"
    char ip[16];         // Dirección IP del worker
    int port;            // Puerto del worker
    int available;       // 1 = Disponible, 0 = Ocupado
} Worker;

// This is the server
Gotham gotham;
int data_file_fd, listenFleckFD, listenWorkerFD = 0;
pthread_t FleckThread, DistorsionWorkersThread;
int terminate = FALSE;
Worker workers[MAX_WORKERS];
int workerCount = 0;
Worker *primaryHarley = NULL;
Worker *primaryEnigma = NULL;
// Mutex para sincronización
pthread_mutex_t workersMutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Saves the information of the Gotham file into the Gotham struct
 *  @param filename The name of the file to read
 */
void saveGotham(char *filename)
{
    data_file_fd = open(filename, O_RDONLY);
    if (data_file_fd < 0)
    {
        printError("Error while opening the Fleck file\n");
        exit(1);
    }

    gotham.fleck_ip = readUntil('\n', data_file_fd);
    char *port1 = readUntil('\n', data_file_fd);
    gotham.fleck_port = atoi(port1);
    free(port1);

    gotham.harley_enigma_ip = readUntil('\n', data_file_fd);
    char *port = readUntil('\n', data_file_fd);
    gotham.harley_enigma_port = atoi(port);
    free(port);
    printToConsole("\n\nReading configuration file\n\n");
}
/**
 * @brief Closes the file descriptors if they are open
 */
void closeFds()
{
    if (data_file_fd != 0)
    {
        close(data_file_fd);
    }
    if (listenFleckFD != 0)
    {
        close(listenFleckFD);
    }
    if (listenWorkerFD != 0)
    {
        close(listenWorkerFD);
    }
}
/**
 * @brief Frees the memory allocated for the Gotham struct
 */
void freeMemory()
{
    free(gotham.fleck_ip);
    free(gotham.harley_enigma_ip);
}
/**
 * @brief Libera un worker para que vuelva a estar disponible.
 *
 * @param ip Dirección IP del worker.
 * @param port Puerto del worker.
 */
void releaseWorker(const char *ip, int port)
{
    pthread_mutex_lock(&workersMutex);

    for (int i = 0; i < workerCount; i++)
    {
        if (strcmp(workers[i].ip, ip) == 0 && workers[i].port == port)
        {
            workers[i].available = 1; // Marcar como disponible
            printToConsole("Worker marked as available.\n");
            break;
        }
    }

    pthread_mutex_unlock(&workersMutex);
}

/**
 * @brief Closes the program correctly cleaning the memory and closing the file descriptors
 */
void closeProgramSignal()
{
    printToConsole("Gotham shutting down...\n");

    for (int i = 0; i < workerCount; i++)
    {
        releaseWorker(workers[i].ip, workers[i].port);
    }

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

// toDo------------------------------------------------------- U S E R ---------------------------------------------------------------------

/**
 * @brief Marca un worker como ocupado en la lista de workers.
 *
 * @param ip Dirección IP del worker.
 * @param port Puerto del worker.
 */
void busyWorker(const char *ip, int port)
{

    pthread_mutex_lock(&workersMutex); // Bloquea el acceso a la lista de workers

    for (int i = 0; i < workerCount; i++)
    {
        if (strcmp(workers[i].ip, ip) == 0 && workers[i].port == port)
        {
            if (workers[i].available == 1)
            {
                workers[i].available = 0; // Marcar como ocupado
                printf("Worker at IP %s and Port %d marked as busy.\n", ip, port);
            }
            else
            {
                printf("Warning: Worker at IP %s and Port %d is already busy.\n", ip, port);
            }
            break;
        }
    }

    pthread_mutex_unlock(&workersMutex); // Desbloquea el acceso
}

/**
 * @brief Busca un Worker disponible por tipo.
 *
 * @param workerType Tipo de Worker que se busca.
 * @return Worker* Un puntero al Worker disponible, o NULL si no hay disponibles.
 */
Worker *getAvailableWorkerByType(char *workerType)
{
    if (workerType == NULL)
    {
        printError("Worker type is NULL. Cannot search for workers.\n");
        return NULL;
    }

    if (strcmp(workerType, "wav") == 0 || strcmp(workerType, "png") == 0 || strcmp(workerType, "jpg") == 0)
    {
        workerType = "Harley";
    }
    else if (strcmp(workerType, "txt") == 0)
    {
        workerType = "Enigma";
    }

    pthread_mutex_lock(&workersMutex);
    printToConsole("Debugging Worker List:\n");
    // Imprimir la cantidad de workers registrados
    char debugMessage[64];
    snprintf(debugMessage, sizeof(debugMessage), "Número de workers registrados: %d\n", workerCount);
    printToConsole(debugMessage);
    for (int i = 0; i < workerCount; i++)
    {
        char debugMessage[256];
        snprintf(debugMessage, sizeof(debugMessage),
                 "Worker[%d]: Type=%s, IP=%s, Port=%d, Available=%d\n",
                 i, workers[i].workerType, workers[i].ip, workers[i].port, workers[i].available);
        printToConsole(debugMessage);
        // Verificar si el Worker está disponible y coincide con el tipo solicitado
        if (workers[i].available == 1 && strcmp(workers[i].workerType, workerType) == 0)
        {
            workers[i].available = 0; // Marcar como ocupado
            pthread_mutex_unlock(&workersMutex);

            return &workers[i];
        }
    }

    pthread_mutex_unlock(&workersMutex);

    // Log de que no hay Workers disponibles
    char message[256];
    snprintf(message, sizeof(message), "No available workers of type: %s\n", workerType);
    printToConsole(message);

    return NULL; // No hay Workers disponibles
}

/**
 * @brief Maneja una petición de distorsión.
 *
 * @param receivedMessage El mensaje recibido del Fleck.
 * @param clientSocketFD El socket del cliente Fleck.
 */
void handleDistortRequest(SocketMessage receivedMessage, int clientSocketFD)
{

    char *mediaType = strtok(receivedMessage.data, "&");
    char *fileName = strtok(NULL, "&");

    if (mediaType == NULL || fileName == NULL)
    {
        printError("Formato de petición de distorsión incorrecto");
        return;
    }


    // Buscar un Worker disponible del tipo requerido
    printToConsole("Searching for an available worker...\n");
    Worker *worker = getAvailableWorkerByType(mediaType);

    if (!worker)
    {
        printError("No hay trabajadores disponibles para el tipo solicitado.\n");

        // Enviar respuesta de error al cliente
        SocketMessage errorResponse = {0x10, strlen("DISTORT_KO"), strdup("DISTORT_KO"), (unsigned int)time(NULL), 0};
        sendSocketMessage(clientSocketFD, errorResponse);
        free(errorResponse.data);
        return;
    }
    printf("Found available worker: IP = %s, Port = %d\n", worker->ip, worker->port);
    // Marcar el worker como ocupado**
    busyWorker(worker->ip, worker->port);

    //  Construir la respuesta con la información del Worker disponible
    char responseData[256];
    snprintf(responseData, sizeof(responseData), "%s&%d", worker->ip, worker->port);

    SocketMessage successResponse;
    successResponse.type = 0x10;
    successResponse.dataLength = strlen(responseData);
    successResponse.data = strdup(responseData);

    // Imprimir el contenido de successResponse.data
    char message[256];
    snprintf(message, sizeof(message), "Data que envia Gotham a Fleck: %s", successResponse.data);
    printToConsole(message);

    // Enviar respuesta al cliente con la información del Worker
    sendSocketMessage(clientSocketFD, successResponse);

    // Liberar memoria y marcar al Worker como ocupado
    free(successResponse.data);
    printToConsole("\nDistortion request processed successfully.\n");
}

/**
 * @brief Escucha conexiones entrantes de Fleck y redirige las solicitudes a los workers.
 *
 * @param args Argumentos para el hilo (puede ser NULL).
 * @return void* Retorna NULL al finalizar.
 */
void *listenToFleck()
{
    // Crear socket de escucha
    int listenFleckFD = createAndListenSocket(gotham.fleck_ip, gotham.fleck_port);
    if (listenFleckFD < 0)
    {
        printToConsole("Error creando socket de Fleck\n");
        pthread_exit(NULL);
    }
    printToConsole("Waiting for FLECK connections...............\n\n");

    // Inicializar conjuntos de descriptores de archivo
    fd_set master_set, read_set;
    FD_ZERO(&master_set);
    FD_ZERO(&read_set);

    // Agregar el socket de escucha al conjunto maestro
    FD_SET(listenFleckFD, &master_set);
    int max_fd = listenFleckFD; // Máximo descriptor de archivo

    while (1)
    {

        read_set = master_set; // Copiar el conjunto maestro para select()
        // Esperar actividad en los descriptores de archivo
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
                    printf("New Fleck connection accepted: %d\n", newSocketFD);
                    if (newSocketFD > max_fd)
                    {
                        max_fd = newSocketFD; // Actualizar el máximo descriptor de archivo
                    }
                    printToConsole("Nueva conexión de Fleck aceptada.\n");
                }
                else
                {
                    printf("Socket activo: %d\n", fd);
                    // Socket existente - recibir mensaje
                    SocketMessage receivedMessage = getSocketMessage(fd);

                    // Manejar el tipo de mensaje
                    switch (receivedMessage.type)
                    {
                    case 0x01: // Solicitud de conexión
                        // Procesar la trama de Fleck: <userName>&<IP>&<Port>
                        char *userName = strtok(receivedMessage.data, "&");
                        if (userName != NULL)
                        {
                            char message[256];
                            snprintf(message, sizeof(message), "New user connected: %s.\n", userName);
                            printToConsole(message);
                        }
                        else
                        {
                            printToConsole("Error: Invalid user name format.\n");
                            close(fd);
                            FD_CLR(fd, &master_set);
                            continue;
                        }

                        // Responder con ACK de conexión
                        {
                            SocketMessage response = {0x01, 0, NULL, (unsigned int)time(NULL), 0};
                            sendSocketMessage(fd, response);
                        }

                        break;

                    case 0x10: // Petición de distorsión
                        printToConsole("Processing DISTORT request...\n");
                        handleDistortRequest(receivedMessage, fd);
                        break;

                    case 0x11: // Solicitud de reasignación de Worker
                        printToConsole("Processing reassignment request...\n");
                        // handleDistortRequest(receivedMessage, fd);
                        break;

                    case 0x07: // Desconexión
                        char message2[256];
                        snprintf(message2, sizeof(message2), "User disconnected: %s\n", receivedMessage.data);
                        printToConsole(message2);
                        // Liberar el worker correspondiente
                        close(fd);
                        FD_CLR(fd, &master_set);
                        printToConsole("Connection closed.\n");
                        break;

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

// toDo------------------------------------------------------- U S E R ---------------------------------------------------------------------

// toDo-----------------------------------------------------W O R K E R S ------------------------------------------------------------------



/**
 * @brief Reasigna un worker primario si el actual se desconecta.
 *
 * @param workerType El tipo de worker ("Media" o "Text") a reasignar.
 */
void reassignPrimaryWorker(const char *workerType)
{
    pthread_mutex_lock(&workersMutex);

    for (int i = 0; i < workerCount; i++)
    {
        if (strcmp(workers[i].workerType, workerType) == 0 && workers[i].available == 1)
        {
            if (strcmp(workerType, "Media") == 0)
            {
                primaryHarley = &workers[i];
                printToConsole("Primary Harley reassigned.\n");
            }
            else if (strcmp(workerType, "Text") == 0)
            {
                primaryEnigma = &workers[i];
                printToConsole("Primary Enigma reassigned.\n");
            }
            pthread_mutex_unlock(&workersMutex);
            return;
        }
    }

    // Si no hay ningún worker disponible
    if (strcmp(workerType, "Media") == 0)
    {
        primaryHarley = NULL;
        printToConsole("No available Harley workers to reassign.\n");
    }
    else if (strcmp(workerType, "Text") == 0)
    {
        primaryEnigma = NULL;
        printToConsole("No available Enigma workers to reassign.\n");
    }

    pthread_mutex_unlock(&workersMutex);
}

/**
 * @brief Asigna un worker primario si no existe uno actualmente o si el primario se ha caído.
 *
 * @param worker Puntero al worker recién conectado.
 */
void assignPrimaryWorker(Worker *worker)
{
    pthread_mutex_lock(&workersMutex); // Bloquear acceso a la lista de workers

    if (strcmp(worker->workerType, "Harley") == 0)
    {
        if (primaryHarley == NULL || primaryHarley->available == 0)
        {
            primaryHarley = worker;
            printToConsole("Primary Harley assigned.\n");
        }
    }
    else if (strcmp(worker->workerType, "Enigma") == 0)
    {
        if (primaryEnigma == NULL || primaryEnigma->available == 0)
        {
            primaryEnigma = worker;
            printToConsole("Primary Enigma assigned.\n");
        }
    }

    pthread_mutex_unlock(&workersMutex); // Desbloquear acceso
}

/**
 * @brief Registra un nuevo worker en la lista global.
 *
 * @param workerType Tipo de worker recibido (Media o Text, que se mapeará a Harley o Enigma).
 * @param ip Dirección IP del worker.
 * @param port Puerto del worker.
 */
void registerWorker(const char *workerType, const char *ip, int port)
{
    pthread_mutex_lock(&workersMutex);

    if (workerCount >= MAX_WORKERS)
    {
        printError("No se pueden registrar más workers. Límite alcanzado.");
        pthread_mutex_unlock(&workersMutex);
        return;
    }

    // Registrar nuevo worker
    strcpy(workers[workerCount].workerType, workerType);
    strcpy(workers[workerCount].ip, ip);
    workers[workerCount].port = port;
    workers[workerCount].available = 1; // Inicialmente disponible
    workerCount++;

    char message[256];
    snprintf(message, sizeof(message), "Nuevo worker registrado: %s, IP: %s, Puerto: %d, Disponible-1si-0no:%d \n", workerType, ip, port, workers[workerCount - 1].available);
    printToConsole(message);

    pthread_mutex_unlock(&workersMutex);

    // Intentar asignar como worker primario
    assignPrimaryWorker(&workers[workerCount - 1]);
}

void *listenToDistorsionWorkers()
{
    // Crear socket de escucha
    int listenWorkerFD = createAndListenSocket(gotham.harley_enigma_ip, gotham.harley_enigma_port);
    if (listenWorkerFD < 0)
    {
        printToConsole("Error creating workers socket\n");
        pthread_exit(NULL);
    }
    printToConsole("Waiting for WORKER connections...............\n\n");

    // Inicializar conjuntos de descriptores de archivo
    fd_set master_set, read_set;
    FD_ZERO(&master_set);
    FD_ZERO(&read_set);

    // Agregar el socket de escucha al conjunto maestro
    FD_SET(listenWorkerFD, &master_set);
    int max_fd = listenWorkerFD; // Máximo descriptor de archivo

    while (1)
    {

        read_set = master_set; // Copiar el conjunto maestro para select()
        // Esperar actividad en los descriptores de archivo
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
                if (fd == listenWorkerFD)
                {
                    // Nueva conexión entrante
                    int newSocketFD = accept(listenWorkerFD, NULL, NULL);
                    if (newSocketFD < 0)
                    {
                        printError("Error al aceptar conexión de Fleck\n");
                        continue;
                    }

                    // Agregar el nuevo socket al conjunto maestro
                    FD_SET(newSocketFD, &master_set);
                    printf("New socket: %d\n", newSocketFD);
                    if (newSocketFD > max_fd)
                    {
                        max_fd = newSocketFD; // Actualizar el máximo descriptor de archivo
                    }
                    printToConsole("Nueva conexión de Worker aceptada.\n");
                }
                else
                {
                    printf("Socket: %d\n", fd);
                    // Socket existente - recibir mensaje
                    SocketMessage receivedMessage = getSocketMessage(fd);

                    // Manejar el tipo de mensaje
                    switch (receivedMessage.type)
                    {
                    case 0x02:
                        // Procesar la trama de Worker: <workerType
                        char *workerType = strtok(receivedMessage.data, "&");
                        char *ip = strtok(NULL, "&");
                        char *portStr = strtok(NULL, "&");

                        int port = atoi(portStr); // Convertir el puerto a entero

                        // Mensaje de nuevo worker conectado
                        char *mappedType = NULL;

                        if (strcasecmp(workerType, "Media") == 0)
                        {
                            mappedType = "Harley";
                        }
                        else if (strcasecmp(workerType, "Text") == 0)
                        {
                            mappedType = "Enigma";
                        }
                        else
                        {
                            printError("Tipo de worker inválido. Ignorando registro.");
                            free(receivedMessage.data);
                            close(fd);
                        }

                        registerWorker(mappedType, ip, port);

                        char message[256];
                        snprintf(message, sizeof(message), "NEW %s worker connected - ready to distort!\n", mappedType);
                        printToConsole(message);


                        // Responder con una trama OK
                        SocketMessage response;
                        response.type = 0x02;
                        response.dataLength = 0;
                        response.data = NULL; // Trama OK con DATA vacío

                        sendSocketMessage(fd, response);
                        printToConsole("Worker registered successfully!!!!!!!!\n");
                        break;

                        
                        default: // Tipo no reconocido
                        printToConsole("Tipo de mensaje no reconocido. Enviando trama de error...\n");
                        {
                            SocketMessage errorResponse = {0x09, 0, NULL, (unsigned int)time(NULL), 0};
                            sendSocketMessage(fd, errorResponse);
                        }
                        break;

                    case 0x07:
                        // Desconexión
                        close(fd);
                        FD_CLR(fd, &master_set);
                        printToConsole("Conexión de WORKER cerrada.\n");
                        break;
                    }

                    // Liberar recursos del mensaje recibido
                    free(receivedMessage.data);
                }
            }
        }
    }

    close(listenWorkerFD);
    printToConsole("CLOSING WORKER SOCKET\n");
    pthread_exit(NULL);
}
// toDo-----------------------------------------------------W O R K E R S ------------------------------------------------------------------

/**
 * @brief Main function of the Gotham server
 * @param argc The number of arguments
 * @param argv The arguments
 * @return 0 if the program ends correctly
 */
int main(int argc, char *argv[])
{
    initalSetup(argc);

    saveGotham(argv[1]);
    printToConsole("Gotham server initialized\n\n");

    // Retorna 0 si tiene éxito o un código de error si falla
    if (pthread_create(&FleckThread, NULL, (void *)listenToFleck, NULL) != 0)
    {
        printError("Error creating Fleck thread\n");
        exit(1);
    }

    if (pthread_create(&DistorsionWorkersThread, NULL, (void *)listenToDistorsionWorkers, NULL) != 0)
    {
        printError("Error creating Enigma thread\n");
        exit(1);
    }

    pthread_join(FleckThread, NULL);
    pthread_join(DistorsionWorkersThread, NULL);

    freeMemory();
    return 0;
}

