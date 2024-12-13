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
 * @brief Closes the program correctly cleaning the memory and closing the file descriptors
 */
void closeProgramSignal(int sig) {
    printToConsole("Gotham shutting down...\n");

    for (int i = 0; i < workerCount; i++) {
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
 * @brief Busca un worker disponible en la lista global.
 *
 * @return Worker* Un puntero al worker disponible, o NULL si no hay disponibles.
 */
Worker *getAvailableWorker()
{
    pthread_mutex_lock(&workersMutex);

    for (int i = 0; i < workerCount; i++)
    {
        if (workers[i].available == 1) // Si el worker está disponible
        {
            workers[i].available = 0; // Marcar como ocupado
            pthread_mutex_unlock(&workersMutex);
            return &workers[i];
        }
    }

    pthread_mutex_unlock(&workersMutex);
    return NULL; // No hay workers disponibles
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

//---------------------------------- U S E R ------------------------------------
/**
 * @brief Redirige una petición de usuario Fleck al worker Harley.
 *
 * @param clientSocketFD Socket del cliente (Fleck).
 * @param userName Nombre del usuario (ej. Arthur).
 * @param ip Dirección IP del usuario.
 * @param port Puerto del usuario.
 */
void redirectToWorker(int clientSocketFD, char *userName, char *ip, int port)
{
    // Buscar un worker Harley disponible
    int workerFD = getAvailableWorker("Harley");
    if (workerFD < 0)
    {
        printError("No hay workers Harley disponibles para atender la petición");
        return;
    }

    // Crear la trama para enviar al worker Harley
    SocketMessage workerMessage;
    workerMessage.type = 0x02;                                     // Tipo de mensaje para worker
    workerMessage.dataLength = strlen(userName) + strlen(ip) + 10; // Calcular tamaño dinámico
    workerMessage.data = malloc(workerMessage.dataLength + 1);
    snprintf(workerMessage.data, workerMessage.dataLength + 1, "%s&%s&%d", userName, ip, port);
    workerMessage.timestamp = (unsigned int)time(NULL);
    workerMessage.checksum = calculateChecksum(workerMessage.data, workerMessage.dataLength);

    sendSocketMessage(workerFD, workerMessage);
    free(workerMessage.data);

    // Confirmar en consola
    printToConsole("New Harley worker connected - ready to distort!!!!!!!!\n");
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

    while (1)
    {
        // Aceptar conexión entrante
        int fleckSocketFD = accept(listenFleckFD, (struct sockaddr *)NULL, NULL);
        if (fleckSocketFD < 0)
        {
            printToConsole("Error al aceptar conexión de Fleck\n");
            continue;
        }

        // Recibir mensaje de Fleck
        SocketMessage receivedMessage = getSocketMessage(fleckSocketFD);
        if (receivedMessage.data == NULL || receivedMessage.type != 0x01)
        {
            printToConsole("Error en el formato del mensaje recibido de Fleck\n");
            free(receivedMessage.data);
            close(fleckSocketFD);
            continue;
        }

        // Procesar la trama de Fleck: <userName>&<IP>&<Port>
        char *userName = strtok(receivedMessage.data, "&");
        char *userIP = strtok(NULL, "&");
        char *userPortStr = strtok(NULL, "&");

        if (userName && userIP && userPortStr)
        {

            char message[MAX_MESSAGE_SIZE];
            snprintf(message, sizeof(message), "New user connected: %s.\n", userName);
            printToConsole(message);

            // Obtener un worker disponible
            Worker *availableWorker = getAvailableWorker();

            if (availableWorker != NULL)
            {
                snprintf(message, sizeof(message), "Redirigiendo a %s worker en IP: %s, Puerto: %d\n",
                         availableWorker->workerType, availableWorker->ip, availableWorker->port);
                printToConsole(message);
                releaseWorker(availableWorker->ip, availableWorker->port);
            }
            else
            {
                // snprintf(message, sizeof(message), "No hay workers disponibles para atender a %s en este momento.\n", userName);
                // printToConsole(message);
                //  Sin Workers disponibles
                const char *errorMessage = "No workers available.";
                SocketMessage response = {0x09, strlen(errorMessage), strdup(errorMessage), (unsigned int)time(NULL), 0};
                sendSocketMessage(fleckSocketFD, response);
                free(response.data);
            }
        }
        else
        {
            printToConsole("Formato de datos incorrecto recibido de Fleck.\n");
        }

        // Liberar recursos y cerrar el socket
        free(receivedMessage.data);
        close(fleckSocketFD);
    }

    close(listenFleckFD);
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
    if (strcmp(worker->workerType, "Harley") == 0 && primaryHarley == NULL)
    {
        primaryHarley = worker;
        printToConsole("Primary Harley assigned.\n");
    }
    else if (strcmp(worker->workerType, "Enigma") == 0 && primaryEnigma == NULL)
    {
        primaryEnigma = worker;
        printToConsole("Primary Enigma assigned.\n");
    }
}

/**
 * @brief Procesa una solicitud en función del tipo de worker.
 *
 * @param workerType Tipo de worker (por ejemplo, "Harley", "Enigma").
 * @param ip Dirección IP del worker.
 * @param port Puerto del worker.
 */
void processFileBasedOnWorkerType(const char *workerType, const char *ip, int port)
{
    if (strcmp(workerType, "Harley") == 0)
    {

        // char message[256];
        //  snprintf(message, sizeof(message), "New %s worker connected – ready to distort!\n", workerType);
        //  printToConsole(message);
        printToConsole("Procesando distorsión del archivo por Harley\n");
        // Implementar la lógica específica para Harley aquí
    }
    else if (strcmp(workerType, "Enigma") == 0)
    {
        // char message[256];
        //  snprintf(message, sizeof(message), "New %s worker connected – ready to distort!\n", workerType);
        //  printToConsole(message);
        printToConsole("Procesando distorsión del archivo por Enigma\n");
        // Implementar la lógica específica para Enigma aquí
    }
    else
    {
        printError("Tipo de worker desconocido");
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
            snprintf(message, sizeof(message), "Nuevo %s worker conectado - listo para distorsionar!\n", workerType);
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
