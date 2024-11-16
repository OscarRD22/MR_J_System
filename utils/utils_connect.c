#include "utils_connect.h"
#include "io_utils.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include "../struct_definitions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

// Osar.romero - Marc.marza

/**
 * @brief Gets a message from a socket
 * @param clientFD The socket file descriptor
 * @return The message
 */
SocketMessage getSocketMessage(int clientFD)
{
    char *buffer;
    SocketMessage message;
    ssize_t numBytes;

    // get the type
    uint8_t type;
    numBytes = read(clientFD, &type, 1);
    if (numBytes < 1)
    {
        printError("Error reading the type\n");
    }
    asprintf(&buffer, "Type: 0x%02x\n", type);
    printToConsole(buffer);
    free(buffer);
    message.type = type;

    // get the data length
    uint16_t dataLength;
    numBytes = read(clientFD, &dataLength, sizeof(unsigned short));
    if (numBytes < (ssize_t)sizeof(unsigned short))
    {
        printError("Error reading the Data Length\n");
    }
    asprintf(&buffer, "Data length: %u\n", dataLength);
    printToConsole(buffer);
    free(buffer);
    message.dataLength = dataLength;

    // get the data
    char *data = malloc(sizeof(char) * 256 - 9 - dataLength);
    numBytes = read(clientFD, data, 256 - 9 - dataLength);

    if (numBytes < 256 - 9 - dataLength)
    {
        printError("Error reading the data\n");
        free(data);
    }
    else
    {
        message.data = malloc(sizeof(char) * numBytes);
        memcpy(message.data, data, numBytes);
    }
    asprintf(&buffer, "Data: %s\n", data);
    printToConsole(buffer);
    free(buffer);

    // get the checksum 2bytes
    char checksum[2];
    numBytes = read(clientFD, checksum, 2);
    if (numBytes < 2)
    {
        printError("Error reading the checksum\n");
    }
    asprintf(&buffer, "Checksum: %c%c\n", checksum[0], checksum[1]);
    printToConsole(buffer);
    free(buffer);
    // Falta castear el message.checksum
    // message.checksum = checksum;

    // get the timestamp
    char timestamp[4];
    numBytes = read(clientFD, timestamp, 4);
    if (numBytes < 4)
    {
        printError("Error reading the timestamp\n");
    }
    asprintf(&buffer, "Timestamp: %c%c%c%c\n", timestamp[0], timestamp[1], timestamp[2], timestamp[3]);
    printToConsole(buffer);
    free(buffer);
    message.timestamp[0] = timestamp[0];
    message.timestamp[1] = timestamp[1];
    message.timestamp[2] = timestamp[2];
    message.timestamp[3] = timestamp[3];

    return message;
}

/**
 * @brief Creates a socket and connects it to a server
 * @param IP The IP address of the server
 * @param port The port of the server
 * @return The socket file descriptor
 */
int createAndConnectSocket(char *IP, int port, int isVerbose)
{
    char *buffer;
    if (isVerbose == TRUE)
    {
        asprintf(&buffer, "Creating and connecting socket on %s:%d\n", IP, port);
        printToConsole(buffer);
        free(buffer);
    }

    int socketFD;
    struct sockaddr_in server;

    if ((socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        printError("Error creating the socket\n");
        exit(1);
    }

    bzero(&server, sizeof(server));
    server.sin_port = htons(port);
    server.sin_family = AF_INET;

    // Check if the IP is valid and if it failed to convert, check why
    if (inet_pton(AF_INET, IP, &server.sin_addr) <= 0)
    {
        if (isVerbose == TRUE)
        {
            asprintf(&buffer, "IP address: %s\n", IP);
            printToConsole(buffer);
            free(buffer);
        }

        if (inet_pton(AF_INET, IP, &server.sin_addr) == 0)
        {
            printError("inet_pton() failed: invalid address string\n");
        }
        else
        {
            printError("inet_pton() failed\n");
        }
        printError("Error converting the IP address\n");
        exit(1);
    }

    // Connect to server
    if (connect(socketFD, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        printError("Error connecting\n");
        exit(1);
    }

    return socketFD;
}

/**
 * @brief Creates a socket and listens to it
 * @param IP The IP address of the server
 * @param port The port of the server
 * @return The socket file descriptor
 */
int createAndListenSocket(char *IP, int port)
{
    char *buffer;
    asprintf(&buffer, "Creating socket on %s:%d\n", IP, port);
    printToConsole(buffer);
    free(buffer);

    int socketFD;
    struct sockaddr_in server;

    if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printError("Error creating the socket\n");
        exit(1);
    }

    bzero(&server, sizeof(server));
    server.sin_port = htons(port);
    server.sin_family = AF_INET;

    // Check if the IP is valid and if it failed to convert, check why
    if (inet_pton(AF_INET, IP, &server.sin_addr) <= 0)
    {
        char *buffer;
        asprintf(&buffer, "IP address: %s\n", IP);
        printToConsole(buffer);
        free(buffer);

        if (inet_pton(AF_INET, IP, &server.sin_addr) == 0)
        {
            printError("inet_pton() failed: invalid address string\n");
        }
        else
        {
            printError("inet_pton() failed\n");
        }
        printError("Error converting the IP address\n");
        exit(1);
    }

    printToConsole("Socket created\n");

    if (bind(socketFD, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        printError("Error binding the socket\n");
        exit(1);
    }

    printToConsole("Socket binded\n");

    if (listen(socketFD, 10) < 0)
    {
        printError("Error listening\n");
        exit(1);
    }

    return socketFD;
}

/**
 * @brief Sends a message through a socket this does not work with sending files
 * @param socketFD The socket file descriptor
 * @param message The message to send
 */
void sendSocketMessage(int socketFD, SocketMessage message)
{
    char *buffer = malloc(sizeof(char) * 256);
    buffer[0] = message.type;
    buffer[1] = (message.dataLength & 0xFF);
    buffer[2] = ((message.dataLength >> 8) & 0xFF);

    size_t i;
    int start_i = 3;
    for (i = 0; i < strlen(message.data) && message.data != NULL; i++)
    {
        buffer[i + start_i] = message.data[i];
        // printf("buffer);
    }

    int start_j = strlen(message.data) + start_i;
    for (int j = 0; j < (256 - 9 - strlen(message.data) + 1); j++)
    {
        buffer[j + start_j] = '@';
        printf("buffer[%d] = %c\n", j + start_j, buffer[j + start_j]);
    }

    // Enviar el checksum esta harcode
    buffer[251] = 'C';
    buffer[252] = 'H';

    // Enviar el timestamp esta harcode
    buffer[253] = 'T';
    buffer[254] = 'I';
    buffer[255] = 'M';
    buffer[256] = 'E';

    write(socketFD, buffer, 256);
    free(buffer);
}
