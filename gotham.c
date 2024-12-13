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
    char workerType[10]; // Tipo: "Media" o "Text"
    char ip[16];         // Dirección IP del worker
    int port;            // Puerto del worker
    int available;       // 1 = Disponible, 0 = Ocupado
} Worker;

// This is the client
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
 * ANTIGUA FUNCION
 * @brief Listens to the Fleck server and sends the information of the workers server with less Flecks connected

void *listenToFleck()
{
    // printToConsole("Listening to Fleck\n\n");

    if ((listenFleckFD = createAndListenSocket(gotham.fleck_ip, gotham.fleck_port)) < 0)
    {
        printError("Error creating Fleck socket\n");
        exit(1);
    }
    // INICIALIZAMOS EL SET DE SOCKETS
    // fd_set es un conjunto de descriptores de fichero
    fd_set read_set, ready_sockets, write_set;

    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_SET(listenFleckFD, &read_set);
    int max_fd = 10; // CAMBIAR SEGUN REQURIEMIENTOS
    while (terminate == FALSE)
    {
        printToConsole("Waiting for connections... (MSG in SELECT) \n\n");
        // VAMOS A TENER NUESTRO SELECT EL SELECT ES DESTUCTIVO
        ready_sockets = read_set;

        int activity = select(max_fd + 1, &ready_sockets, &write_set, NULL, NULL); // NO HACEMOS POLLING AUN
        if (activity < 0)
        {
            printError("Error in select\n");
            exit(1);
        }
        // SELECT CREADO Y FUNCIONANDO ---------------------------------------------

        for (int i = 0; i < max_fd + 1; i++)
        {
            if (FD_ISSET(i, &ready_sockets)) // SI EL SOCKET ESTA EN EL SET (LISTA DE SOCKETS)
            {
                if (i == listenFleckFD)
                {                                                                                // SOCKET DE ESCUCHA DE FLECK's
                    int connectionHandle = accept(listenFleckFD, (struct sockaddr *)NULL, NULL); // IGUAL ES i EN VEZ DE listenFleckFD antes era listenFleckFD
                    if (connectionHandle < 0)
                    {
                        printError("Error accepting Fleck\n");
                        exit(1);
                    }
                    FD_SET(connectionHandle, &read_set);

                    SocketMessage m = getSocketMessage(connectionHandle);
                    // Extraer el nombre de Fleck desde m.data
                    char *token = strtok(m.data, "&"); // Dividir la cadena usando '&' como delimitador
                    if (token != NULL)
                    {
                        printToConsole("New user connected: ");
                        printToConsole(token);
                        printToConsole("\n");
                    }

                    // Liberar la memoria de m.data si es necesario
                    free(m.data);

                    if (m.type == 0x01)
                    {

                        printToConsole("SENDING RESPONSE TO FLECK");
                        SocketMessage r;
                        r.type = 0x01;
                        r.dataLength = 0;
                        r.data = strdup("");

                        sendSocketMessage(connectionHandle, r);
                        free(r.data);
                    }
                }
                else
                { // CLIENETES CONECTADOS
                    // NO ES EL SOCKET DE ESCUCHA DE FLECK
                    // ES UN SOCKET DE UN FLECK CONECTADO
                    // TENEMOS QUE HANDLEAR LOS MENSAJES QUE RECIBIMOS
                    printToConsole("Fleck connected\n");
                    SocketMessage m = getSocketMessage(i);
                    if (m.type == 0x10)
                    {
                        printToConsole("Fleck requested DISTORSION\n");
                        // BUSCAR SERVER DISPOINIBLE
                        // ENVIAR IP Y PUERTO DEL SERVER DISPOINIBLE
                    }
                    else if (m.type == 0x07)
                    {
                        printToConsole("FLECK REQUESTED DISCONNECTION\n");

                        // CERRAMOS EL SOCKET
                        FD_CLR(i, &read_set);
                        close(i);
                    }
                    else
                    {
                        // ENVIAR 0x09
                        sendError(i);
                    }

                    // FALTA EL FD CLEAR DE LOS SOCKETS QUE SE HAN DESCONECTADO
                }
            }
        }
    }
    return NULL;
}
*/

/**
 * @brief Busca un Worker disponible por tipo.
 *
 * @param workerType Tipo de Worker que se busca.
 * @return Worker* Un puntero al Worker disponible, o NULL si no hay disponibles.
 */
Worker *getAvailableWorkerByType(const char *workerType)
{
    pthread_mutex_lock(&workersMutex);

    for (int i = 0; i < workerCount; i++)
    {
        if (workers[i].available == 1 && strcmp(workers[i].workerType, workerType) == 0)
        {
            workers[i].available = 0; // Marcar como ocupado
            pthread_mutex_unlock(&workersMutex);
            return &workers[i];
        }
    }

    pthread_mutex_unlock(&workersMutex);
    return NULL; // No hay Workers disponibles
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
            break;
        }
    }

    pthread_mutex_unlock(&workersMutex);
}

/**
 * @brief Closes the program correctly cleaning the memory and closing the file descriptors
 */
void closeProgramSignal(int sig)
{
    printToConsole("Gotham shutting down...\n");

    for (int i = 0; i < workerCount; i++)
    {
        releaseWorker(workers[i].ip, workers[i].port);
    }

    closeFds();
    freeMemory();
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
//---------------------------------- U S E R ------------------------------------
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


    //AQUI SE DEBE DE BUSCAR EL WORKER DISPONIBLE Y ENVIARLE LA INFORMACION PARA QUE SE CONECTE pero falla.
    Worker *worker = getAvailableWorkerByType(mediaType);

    printToConsole(worker->workerType);
    SocketMessage response;
    response.type = 0x10;

    char *responsedata;
    if (worker != NULL)
    {
        printToConsole("Processing distortion request...\n");
        asprintf(&responsedata, "%s&%d", worker->ip, worker->port);
        response.dataLength = strlen(responsedata);
        response.data = strdup(responsedata);
        releaseWorker(worker->ip, worker->port);
    }
    else
    {
        printToConsole("Gotham could not process the distortion request. No workers\n");
        response.dataLength = strlen("DISTORT_KO");
        response.data = strdup("DISTORT_KO");
    }

    response.timestamp = (unsigned int)time(NULL);
    response.checksum = calculateChecksum(response.data, response.dataLength);
    sendSocketMessage(clientSocketFD, response);

    printToConsole("Gotham me ha respondido\n");
    free(response.data);
}

/**
 * @brief Escucha conexiones entrantes de Fleck y redirige las solicitudes a los workers.
 *
 * @param args Argumentos para el hilo (puede ser NULL).
 * @return void* Retorna NULL al finalizar.
 */
void *listenToFleck(void *args)
{
    // Crear socket de escucha
    int listenFleckFD = createAndListenSocket(gotham.fleck_ip, gotham.fleck_port);
    if (listenFleckFD < 0)
    {
        printToConsole("Error creando socket de Fleck\n");
        pthread_exit(NULL);
    }
    printToConsole("Waiting for connections...\n\n");

    // Inicializar conjuntos de descriptores de archivo
    fd_set master_set, read_set;
    FD_ZERO(&master_set);
    FD_ZERO(&read_set);

    // Agregar el socket de escucha al conjunto maestro
    FD_SET(listenFleckFD, &master_set);
    int max_fd = listenFleckFD; // Máximo descriptor de archivo

    while (1)
    {

        read_set = master_set; // Copiar el conjunto maestro para `select()`

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
                    int newSocketFD = accept(listenFleckFD, (struct sockaddr *)NULL, NULL);
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
                    // Socket existente - recibir mensaje
                    SocketMessage receivedMessage = getSocketMessage(fd);
                    if (receivedMessage.data == NULL)
                    {
                        printToConsole("Error al recibir datos. Cerrando conexión...\n");
                        close(fd);
                        FD_CLR(fd, &master_set);
                        continue;
                    }

                    char message[256];
                    // Manejar el tipo de mensaje
                    switch (receivedMessage.type)
                    {
                    case 0x01: // Solicitud de conexión
                        // Procesar la trama de Fleck: <userName>&<IP>&<Port>
                        char *userName = strtok(receivedMessage.data, "&");
                        if (userName != NULL)
                        {
                            snprintf(message, sizeof(message), "New user connected: %s.\n", userName);
                            printToConsole(message);
                        }
                        else
                        {
                            printToConsole("Error al procesar el nombre del usuario.\n");
                        }

                        // Responder con ACK de conexión
                        {
                            SocketMessage response = {0x01, 0, NULL, (unsigned int)time(NULL), 0};
                            sendSocketMessage(fd, response);
                        }
                        break;

                    case 0x10: // Petición de distorsión
                        //printToConsole("Processing distortion request...\n");
                        handleDistortRequest(receivedMessage, fd);
                        break;

                    case 0x11: // Solicitud de reasignación de Worker
                        printToConsole("Processing reassignment request...\n");
                        handleDistortRequest(receivedMessage, fd);
                        break;

                    case 0x07: // Desconexión
                        snprintf(message, sizeof(message), "User disconnected: %s\n", receivedMessage.data);
                        printToConsole(message);

                        // Responder con ACK de desconexión
                        {
                            SocketMessage response = {0x07, 0, NULL, (unsigned int)time(NULL), 0};
                            sendSocketMessage(fd, response);
                        }

                        // Eliminar socket del conjunto maestro y cerrarlo
                        close(fd);
                        FD_CLR(fd, &master_set);
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

//---------------------------------- U S E R ------------------------------------

//---------------------------------- W O R K E R S ------------------------------------

/**
 * @brief Listens to the Workers distorsion server Enigma/Harley and adds it to the list "Antigua funcion BORRAR"

void *listenDistorsionWorkers()
{

    // Crear socket de escucha
    if ((listenWorkerFD = createAndListenSocket(gotham.harley_enigma_ip, gotham.harley_enigma_port)) < 0)
    {
        printError("Error creating workers socket\n");
        pthread_exit(NULL);
        exit(1);
    }

    // Inicialización del conjunto de sockets
    fd_set read_set, ready_sockets;
    FD_ZERO(&read_set);                // Limpia el conjunto
    FD_SET(listenWorkerFD, &read_set); // Agrega el socket de escucha al conjunto
    int max_fd = listenWorkerFD;       // Inicializar el valor máximo de FD

    while (terminate == FALSE)
    {
        // Copiamos el conjunto de sockets porque `select` lo modifica
        ready_sockets = read_set;

        // Llamada a `select` para monitorear actividad
        int activity = select(max_fd + 1, &ready_sockets, NULL, NULL, NULL);
        if (activity < 0)
        {
            printError("Error in W select\n");
            exit(1);
        }

        // Iteramos por todos los posibles sockets activos
        for (int i = 0; i <= max_fd; i++)
        {
            if (FD_ISSET(i, &ready_sockets)) // Si el socket está activo
            {
                if (i == listenWorkerFD) // Nueva conexión de un trabajador
                {
                    int workerSocketFD = accept(listenWorkerFD, (struct sockaddr *)NULL, NULL);
                    if (workerSocketFD < 0)
                    {
                        printError("Error accepting worker\n");
                        continue;
                    }

                    // Recibir el mensaje del trabajador para extraer su tipo
                    SocketMessage m = getSocketMessage(workerSocketFD);

                    char *workerType = strtok(m.data, "&"); // Extraer el tipo de trabajador

                    if (workerType != NULL)
                    {
                        // Añadir el trabajador a la lista
                        addWorker("Worker", workerType, gotham.harley_enigma_ip, gotham.harley_enigma_port);
                        // Imprimir los trabajadores
                        for (int i = 0; i < numOfWorkers; i++)
                        {
                            printf("Trabajador %d: %s, Tipo: %s, IP: %s, Puerto: %d\n",
                                   i + 1,
                                   workers[i].workerName,
                                   workers[i].workerType,
                                   workers[i].WorkerIP,
                                   workers[i].WorkerPort);
                        }
                        // Responder al cliente con una confirmación
                        SocketMessage response;
                        response.type = 0x02; // Tipo de respuesta
                        response.dataLength = 0;
                        response.data = NULL;
                        sendSocketMessage(workerSocketFD, response);
                        char message[256];
                        snprintf(message, sizeof(message), "New %s worker connected - ready to distort!\n", workerType);
                        printToConsole(message);
                    }
                    else
                    {
                        printError("Invalid message format from worker\n");
                    }

                    // Agregamos el nuevo socket al conjunto de lectura
                    FD_SET(workerSocketFD, &read_set);
                    if (workerSocketFD > max_fd)
                        max_fd = workerSocketFD; // Actualizamos el FD máximo
                }
                else // Mensaje de un trabajador existente
                {
                    // Buscar el trabajador que envió el mensaje
                    WorkerServer *worker = NULL;
                    for (int i = 0; i < numOfWorkers; i++)
                    {
                        if (workers[i].WorkerPort == i)
                        {
                            worker = &workers[i];
                            break;
                        }
                    }

                    if (worker == NULL)
                    {
                        printError("Worker not found\n");
                        continue;
                    }

                    // Recibir el mensaje del trabajador
                    SocketMessage m = getSocketMessage(i);

                    // Procesar el mensaje según el tipo
                    if (m.type == 0x10) // Petición de distorsión
                    {
                        // Enviar la IP y puerto de un Fleck con menos conexiones

                        // Buscar el Fleck con menos conexiones
                        // Enviar la IP y puerto del Fleck al trabajador
                        // Enviar un mensaje de confirmación al trabajador
                    }
                    else if (m.type == 0x07) // Petición de desconexión
                    {
                        // Cerrar el socket del trabajador
                        FD_CLR(i, &read_set);
                        close(i);
                        printToConsole("Worker disconnected\n");
                    }
                    else
                    {
                        // Enviar un mensaje de error al trabajador
                        sendError(i);
                    }
                }
            }
        }
    }
    // Liberar memoria
    freeWorkers();
    return NULL;
}

 */

/**
 * @brief Registra un nuevo worker en la lista global.
 *
 * @param workerType Tipo de worker (Harley o Enigma).
 * @param ip Dirección IP del worker.
 * @param port Puerto del worker.
 */
void registerWorker(const char *workerType, const char *ip, int port)
{
    pthread_mutex_lock(&workersMutex);

    if (workerCount < MAX_WORKERS)
    {
        strcpy(workers[workerCount].workerType, workerType);
        strcpy(workers[workerCount].ip, ip);
        workers[workerCount].port = port;
        workers[workerCount].available = 1; // Inicialmente disponible
        workerCount++;

        char message[256];
        snprintf(message, sizeof(message), "Nuevo worker registrado: %s, IP: %s, Puerto: %d\n", workerType, ip, port);
        printToConsole(message);
    }
    else
    {
        printError("No se pueden registrar más workers. Límite alcanzado.");
    }

    pthread_mutex_unlock(&workersMutex);
}

/**
 * @brief Asigna un worker primario para Harley o Enigma si no existen.
 */
void assignPrimaryWorker(Worker *worker)
{
    // No es media ha de ser Harley
    if (strcmp(worker->workerType, "Media") == 0 && primaryHarley == NULL)
    {
        primaryHarley = worker;
        printToConsole("Primary Harley assigned.\n");
    }
    else if (strcmp(worker->workerType, "Text") == 0 && primaryEnigma == NULL)
    {
        primaryEnigma = worker;
        printToConsole("Primary Enigma assigned.\n");
    }
}

/**
 * @brief Escucha conexiones entrantes de los workers y responde según el protocolo.
 *
 * @param args Argumentos para el hilo (puede ser NULL).
 * @return void* Retorna NULL al finalizar.
 */
void *listenToDistorsionWorkers(void *args)
{
    // Crear socket de escucha
    int listenWorkerFD = createAndListenSocket(gotham.harley_enigma_ip, gotham.harley_enigma_port);
    if (listenWorkerFD < 0)
    {
        printToConsole("Error creating workers socket\n");
        pthread_exit(NULL);
    }

    while (1)
    {
        // Aceptar conexión entrante
        int workerSocketFD = accept(listenWorkerFD, (struct sockaddr *)NULL, NULL);
        if (workerSocketFD < 0)
        {
            printToConsole("Error al aceptar conexión de worker\n");
            continue;
        }

        // Recibir mensaje del worker
        SocketMessage receivedMessage = getSocketMessage(workerSocketFD);
        if (receivedMessage.data == NULL || receivedMessage.type != 0x02)
        {
            printToConsole("Error en el formato del mensaje recibido\n");
            free(receivedMessage.data);
            close(workerSocketFD);
            continue;
        }

        // Procesar la trama: <workerType>&<IP>&<Port>
        char *workerType = strtok(receivedMessage.data, "&");
        char *ip = strtok(NULL, "&");
        char *portStr = strtok(NULL, "&");

        if (workerType && ip && portStr)
        {
            int port = atoi(portStr); // Convertir el puerto a entero
            registerWorker(workerType, ip, port);
            // Asignar Worker primario si no hay ninguno
            Worker *worker = &workers[workerCount - 1];
            assignPrimaryWorker(worker);

            // Responder con una trama OK
            SocketMessage response;
            response.type = 0x02;
            response.dataLength = 0;
            response.data = NULL; // Trama OK con DATA vacío
            response.timestamp = (unsigned int)time(NULL);
            response.checksum = 0;

            sendSocketMessage(workerSocketFD, response);

            char message[256];
            snprintf(message, sizeof(message), "NEW %s worker connected - ready to distort!\n", workerType);
            printToConsole(message);
        }
        else
        {
            printToConsole("Formato de datos incorrecto. Respondiendo con CON_KO\n");

            // Responder con una trama KO
            const char *errorMessage = "CON_KO";
            SocketMessage errorResponse;
            errorResponse.type = 0x02;
            errorResponse.dataLength = strlen(errorMessage);
            errorResponse.data = strdup(errorMessage);
            errorResponse.timestamp = (unsigned int)time(NULL);
            errorResponse.checksum = calculateChecksum(errorResponse.data, errorResponse.dataLength);

            sendSocketMessage(workerSocketFD, errorResponse);

            free(errorResponse.data);
        }

        // Liberar recursos y cerrar el socket del worker
        free(receivedMessage.data);
        close(workerSocketFD);
    }

    close(listenWorkerFD);
    pthread_exit(NULL);
}
//---------------------------------- W O R K E R S ------------------------------------

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
