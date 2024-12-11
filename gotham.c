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

// Osar.romero - Marc.marza

// This is the client
Gotham gotham;
int data_file_fd, listenFleckFD, listenEnigmaFD, listenHarleyFD = 0;
pthread_t FleckThread, DistorsionWorkersThread;
int terminate = FALSE;
int numOfWorkers = 0;

//
WorkerServer *workers; // Array of workers

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
    if (listenEnigmaFD != 0)
    {
        close(listenEnigmaFD);
    }
    if (listenHarleyFD != 0)
    {
        close(listenHarleyFD);
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
void closeProgramSignal()
{
    printToConsole("\nClosing program\n");
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
 * @brief Listens to the Fleck server and sends the information of the workers server with less Flecks connected
 */
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

/**
 * @brief Listens to the Workers distorsion server Enigma/Harley and adds it to the list
 */
void *listenToDistorsionWorkers()
{
    // printToConsole("Listening to WORKERS\n\n");

    // Crear socket de escucha
    if ((listenEnigmaFD = createAndListenSocket(gotham.harley_enigma_ip, gotham.harley_enigma_port)) < 0)
    {
        printError("Error creating workers socket\n");
        exit(1);
    }

    // Inicialización del conjunto de sockets
    fd_set read_set, ready_sockets;
    FD_ZERO(&read_set);                // Limpia el conjunto
    FD_SET(listenEnigmaFD, &read_set); // Agrega el socket de escucha al conjunto
    int max_fd = listenEnigmaFD;       // Inicializar el valor máximo de FD

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
                if (i == listenEnigmaFD) // Nueva conexión de un trabajador
                {
                    int workerSocketFD = accept(listenEnigmaFD, (struct sockaddr *)NULL, NULL);
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
                        // Añadir el trabajador a la lista - Falta hacer

                        // Responder al cliente con una confirmación
                        SocketMessage response;
                        response.type = 0x02; // Tipo de respuesta
                        response.dataLength = 0;
                        response.data = strdup("");
                        sendSocketMessage(workerSocketFD, response);
                        free(response.data);
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
                else // Socket de un trabajador existente
                {
                    printToConsole("Worker connected!!!!!\n");
                    // Recibir mensaje del trabajador
                    SocketMessage m = getSocketMessage(i);
                    printToConsole("Worker NO connected!!!!!\n");
                    if (m.type == 0x02) // Ejemplo de tipo de mensaje
                    {
                        printToConsole("Worker requested task assignment\n");
                        // Aquí puedes manejar la lógica específica para el mensaje
                    }
                    else if (m.type == 0x07) // Ejemplo: desconexión
                    {
                        m.dataLength = 0;
                        m.data = strdup(""); // Aqui va el worker type si es necesario
                        printToConsole("Worker disconnected\n");

                        // Limpiar y cerrar el socket
                        FD_CLR(i, &read_set);
                        close(i);
                    }
                }
            }
        }
    }

    return NULL;
}

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
    // printToConsole("Waiting for connections...\n\n");

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
