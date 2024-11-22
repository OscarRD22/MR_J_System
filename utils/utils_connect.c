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

    // Read the type (1 byte)
    numBytes = read(clientFD, &message.type, 1);
    if (numBytes != 1)
    {
        printError("Error reading the message type\n");
        exit(1);
    }

    // Read the data length (2 bytes, little-endian)
    uint16_t dataLength;
    numBytes = read(clientFD, &dataLength, sizeof(uint16_t));
    if (numBytes != sizeof(uint16_t))
    {
        printError("Error reading the data length\n");
        exit(1);
    }
    message.dataLength = dataLength;

    // Read the data

    if (dataLength > 0)
    {
        message.data = malloc(dataLength + 1); // +1 para '\0' si es texto
        if (!message.data)
        {
            printError("Error allocating memory for data\n");
            exit(1);
        }
        numBytes = read(clientFD, message.data, dataLength);
        if (numBytes != dataLength)
        {
            printError("Error reading the message data\n");
            free(message.data);
            exit(1);
        }
        message.data[dataLength] = '\0'; // Asegurar terminación si es texto
    }

    // Read the checksum (2 bytes, little-endian)
    uint16_t checksum;
    numBytes = read(clientFD, &checksum, sizeof(uint16_t));
    if (numBytes != sizeof(uint16_t))
    {
        printError("Error reading the checksum\n");
        free(message.data);
        exit(1);
    }
    message.checksum = checksum;

    // Read the timestamp (4 bytes, little-endian)
    uint32_t timestamp;
    numBytes = read(clientFD, &timestamp, sizeof(uint32_t));
    if (numBytes != sizeof(uint32_t))
    {
        printError("Error reading the timestamp\n");
        free(message.data);
        exit(1);
    }
    message.timestamp = timestamp;

// Debugging logs (optional)
   snprintf(buffer, sizeof(buffer), "Type: 0x%02x\nData length: %u\nChecksum: 0x%04x\nTimestamp: %u\n",
            message.type, message.dataLength, message.checksum, message.timestamp);
   printToConsole(buffer);

   if (message.data) {
       snprintf(buffer, sizeof(buffer), "Data: %s\n", message.data);
       printToConsole(buffer);
   }

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

    //printToConsole("Socket binded\n");

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
    if (!buffer)
    {
        perror("Failed to allocate memory for buffer");
        exit(1);
    }
    // Set type
    buffer[0] = message.type;
    // Set dataLength
    buffer[1] = (message.dataLength & 0xFF);
    buffer[2] = ((message.dataLength >> 8) & 0xFF);

    // Set data
    size_t i;
    for (i = 0; i < message.dataLength && i < 256 - 9; i++)
    {
        buffer[3 + i] = message.data[i];
    }

    // Fill remaining buffer with padding '@'
    for (size_t j = 3 + i; j < 256 - 6; j++)
    { // Hasta antes del checksum y timestamp
        buffer[j] = '@';
    }

    // Calculate and set checksum
    unsigned short checksum = 0;
    for (size_t k = 0; k < 250; k++)
    { // Excluyendo el espacio del checksum y timestamp
        checksum += (unsigned char)buffer[k];
    }
    checksum %= 65536;
    buffer[250] = (checksum & 0xFF); // Byte menos significativo
    buffer[251] = ((checksum >> 8) & 0xFF);

    // Set timestamp
    unsigned int timestamp = (unsigned int)time(NULL); // Epoch time
    buffer[252] = (timestamp & 0xFF);                  // Byte 1
    buffer[253] = ((timestamp >> 8) & 0xFF);           // Byte 2
    buffer[254] = ((timestamp >> 16) & 0xFF);          // Byte 3
    buffer[255] = ((timestamp >> 24) & 0xFF);          // Byte 4

    // Send the buffer through the socket
    if (write(socketFD, buffer, 256) < 0)
    {
        perror("Failed to send data through socket");
    }

    free(buffer);
}
