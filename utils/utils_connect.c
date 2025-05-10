#include "utils_connect.h"
#include "utils_file.h"
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
    ssize_t totalBytesRead = 0, bytesRead;

    message.data = NULL;

    // Lee del socket en un bucle hasta que se reciban todos los datos necesarios.
    while (totalBytesRead < 256)
    {
        bytesRead = read(clientFD, buffer + totalBytesRead, 256 - totalBytesRead);
        if (bytesRead < 0)
        { // Error en la lectura
            printError("Error while reading from socket\n");
            exit(1);
        }
        if (bytesRead == 0)
        { // Desconexión ordenada
            printError("Disconnected\n");
            exit(1);
        }
        totalBytesRead += bytesRead;
    }

    // Deserializa TYPE (1 byte)
    message.type = buffer[0];

    // Deserializa DATA_LENGTH (2 bytes)
    // message.dataLength = buffer[1] | (buffer[2] << 8);
    memcpy(&message.dataLength, &buffer[1], 2);

    // Deserializa DATA (hasta 250 bytes, según DATA_LENGTH)
    if (message.dataLength > 0)
    {
        if (message.dataLength > 247)
        {
            printError("Error: message data length exceeds buffer size\n");
            exit(1);
        }
        message.data = malloc(message.dataLength + 1); // +1 por '\0'
        if (!message.data)
        {
            printError("Error allocating memory for message data\n");
            exit(1);
        }
        memcpy(message.data, &buffer[3], message.dataLength);
        message.data[message.dataLength] = '\0'; // Asegura el final de la cadena
    }

    //printf("Message being 1-GETSocket: Type:%d - DataLength:%d - Data:%s\n", message.type, message.dataLength, message.data);

    // Deserializa y valida CHECKSUM (2 bytes)
    int checksum = buffer[250] | (buffer[251] << 8);
    message.checksum = calculateChecksum(buffer, 250);
    if (checksum != message.checksum)
    {
        printError("Error: checksum mismatch\n");
        // free(message.data);
        // message.data = NULL;
    }
    // else{ printf("Checksum correcto\n");}
    //  Deserializa TIMESTAMP (4 bytes)
    message.timestamp = buffer[252] | (buffer[253] << 8) |
                        (buffer[254] << 16) | (buffer[255] << 24);

    // printf("Message being 2-GETSocket: Type:%d - DataLength:%d - Data:%s - CheckSum:%d - Timestump:%d\n", message.type, message.dataLength, message.data, message.checksum, message.timestamp);

    return message;
}

/**
 * @brief Sends a message through a socket this does not work with sending files
 * @param socketFD The socket file descriptor
 * @param message The message to send
 */
void sendSocketMessage(int socketFD, SocketMessage message)
{
    char buffer[256] = {0}; // Inicialitza el buffer amb 0

    // Serialitza TYPE (1 byte)
    buffer[0] = message.type;

    // Serialitza DATA_LENGTH (2 bytes)
    // buffer[1] = (message.dataLength & 0xFF);         // Byte menys significatiu
    // buffer[2] = ((message.dataLength >> 8) & 0xFF);  // Byte més significatiu
    memcpy(&buffer[1], &message.dataLength, 2);
    // Serialitza DATA (fins a 250 bytes)
    size_t dataSize = (message.dataLength > 247) ? 247 : message.dataLength;
    if (message.data != NULL)
    {
        memcpy(&buffer[3], message.data, dataSize); // Copia els bytes de DATA
    }
    memset(&buffer[3 + dataSize], '\0', 247 - dataSize); // Omple amb '@'

    // Serialitza CHECKSUM (2 bytes)
    int checksum = calculateChecksum(buffer, 250); // Calcula només fins al byte 249
    buffer[250] = (checksum & 0xFF);               // Byte menys significatiu
    buffer[251] = ((checksum >> 8) & 0xFF);        // Byte més significatiu

    // Serialitza TIMESTAMP (4 bytes)
    unsigned int timestamp = (unsigned int)time(NULL);
    buffer[252] = (timestamp & 0xFF);
    buffer[253] = ((timestamp >> 8) & 0xFF);
    buffer[254] = ((timestamp >> 16) & 0xFF);
    buffer[255] = ((timestamp >> 24) & 0xFF);

    //printf("Message being sentSocket: %d - %d - %s - %d - %d\n", message.type, message.dataLength, message.data, checksum, message.timestamp);

    // Envia el buffer pel socket
    if (write(socketFD, buffer, 256) != 256)
    {
        perror("Error sending data through socket");
    }
}

int calculateChecksum(const char *buffer, size_t length)
{
    int checksum = 0;

    // Sumar els valors ASCII dels primers 250 bytes
    for (size_t i = 0; i < length - 6; i++)
    {
        checksum += (unsigned char)buffer[i];
    }

    return checksum % 65536; // Redueix al rang de 16 bits
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
        /*
        asprintf(&buffer, "Creating and connecting socket on %s:%d\n", IP, port);
        printToConsole(buffer);
        free(buffer);
        */
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
    /*
    char *buffer;
    asprintf(&buffer, "Creating socket on %s:%d\n", IP, port);
    printToConsole(buffer);
    free(buffer);
*/
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

    // printToConsole("Socket created\n");

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

void sendError(int socketFD)
{
    SocketMessage message;
    message.type = 0x09;
    message.dataLength = 0;
    message.data = strdup("");
    sendSocketMessage(socketFD, message);
    free(message.data);
}

void sendFile(int socketFD, char *filename)
{

    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        printError("Error opening file\n");
        // sendError(socketFD);
        return;
    }

    fseek(file, 0, SEEK_END);
    fseek(file, 0, SEEK_SET);

    char buffer[247];

    size_t bytesRead = fread(buffer, 1, 247, file);

    while (bytesRead > 0)
    {
        SocketMessage message;
        message.type = 0x05;
        message.dataLength = bytesRead;
        message.data = buffer;
        sendSocketMessage(socketFD, message);
        bytesRead = fread(buffer, 1, 247, file);
    }

    fclose(file);
    return;
}

int compareMD5Sum(char *path, char *md5sum)
{

    if (md5sum == NULL || path == NULL)
    {
        printError("Error: MD5SUM or path is NULL\n");
        return -1;
    }

    char *reciveMD5 = calculateMD5(path);

    if (reciveMD5 == NULL)
    {
        printError("Error: MD5SUM is NULL\n");
        return -1;
    }

    if (strcmp(reciveMD5, md5sum) != 0)
    {
        return -1;
    }

    return 0;
}

void receiveFile(int socketFD, char *filename)
{

    FILE *file = fopen(filename, "w+");
    if (file == NULL)
    {
        printError("Error opening file\n");
        printf("%s\n", filename);
        return;
    }

    SocketMessage message;
    do
    {
        message = getSocketMessage(socketFD);
        if (message.type != 0x05)
        {
            printError("Error: unexpected message type\n");
            sendError(socketFD);
            fclose(file);
            return;
        }
        fwrite(message.data, 1, message.dataLength, file);
    } while (message.dataLength == 247);

    fclose(file);
    return;
}
