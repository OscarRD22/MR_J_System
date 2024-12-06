#include "utils_connect.h"
#include "io_utils.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include "../struct_definitions.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>

// Osar.romero - Marc.marza

/**
 * @brief Gets a message from a socket
 * @param clientFD The socket file descriptor
 * @return The message
 */
SocketMessage getSocketMessage(int clientFD)
{
    SocketMessage message;
    char buffer[256];
    ssize_t numBytes;

    message.data = NULL;

    numBytes = read(clientFD, buffer, 256);
    if (numBytes != 256)
    {
        printError("Error: message size is not 256 bytes\n");
        exit(1);
    }

    message.type = buffer[0];
    message.dataLength = buffer[1] | (buffer[2] << 8);

    if (message.dataLength > 0)
    {
        message.data = malloc(message.dataLength + 1); // +1 para '\0'
        if (!message.data)
        {
            printError("Error allocating memory for message data\n");
            exit(1);
        }
        memcpy(message.data, &buffer[3], message.dataLength);
        message.data[message.dataLength] = '\0';
    }

    message.checksum = buffer[250] | (buffer[251] << 8);
    message.timestamp = buffer[252] | (buffer[253] << 8) |
                        (buffer[254] << 16) | (buffer[255] << 24);

    // Check checksum
    int checksum = 7 + message.dataLength;

    checksum = checksum % 65536;   
    
     // MIRAR CHECKSUM()
    if (checksum != message.checksum)
    {
        printError("Error: checksum mismatch\n");
        free(message.data);
        //exit(1);

    }

    return message;
}

/**
 * @brief Sends a message through a socket this does not work with sending files
 * @param socketFD The socket file descriptor
 * @param message The message to send
 */
void sendSocketMessage(int socketFD, SocketMessage message)
{
    char buffer[256] = {0}; // Inicializo todo a 0

    // Set type
    buffer[0] = message.type;

    // Set dataLength
    buffer[1] = (message.dataLength & 0xFF);
    buffer[2] = ((message.dataLength >> 8) & 0xFF);

    // Set data
    size_t dataSize = (message.dataLength > 250) ? 250 : message.dataLength;
    memcpy(&buffer[3], message.data, dataSize);

    // Fill remaining space with padding '@'
    memset(&buffer[3 + dataSize], '@', 250 - dataSize);

    // Calculate and set checksum
    int checksum = calculateChecksum(buffer, 250);
    buffer[250] = (checksum & 0xFF);
    buffer[251] = ((checksum >> 8) & 0xFF);

    // Set timestamp
    unsigned int timestamp = (unsigned int)time(NULL);
    buffer[252] = (timestamp & 0xFF);
    buffer[253] = ((timestamp >> 8) & 0xFF);
    buffer[254] = ((timestamp >> 16) & 0xFF);
    buffer[255] = ((timestamp >> 24) & 0xFF);

    // Send buffer
    if (write(socketFD, buffer, 256) < 0)
    {
        perror("Error sending data through socket");
    }
}

int calculateChecksum(char *buffer, size_t length)
{
    int checksum = 7 + length;
    checksum = checksum % 65536;
    return checksum;
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
    //toDo Mirar que no sea bloqueante
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

    // printToConsole("Socket binded\n");

    if (listen(socketFD, 10) < 0)
    {
        printError("Error listening\n");
        exit(1);
    }

    return socketFD;
}




void sendError(int socketFD){
    SocketMessage message;
    message.type = 0x09;
    message.dataLength = 0;
    message.data = strdup("");
    sendSocketMessage(socketFD, message);
    free(message.data);
}