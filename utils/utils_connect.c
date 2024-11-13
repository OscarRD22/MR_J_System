#include "utils_connect.h"
#include "io_utils.h"
#include "struct_definitions.h"


// Osar.romero - Marc.marza



/**
 * @brief Gets a message from a socket
 * @param clientFD The socket file descriptor
 * @return The message
 */
SocketMessage getSocketMessage(int clientFD) {
    // char *buffer;
    SocketMessage message;
    ssize_t numBytes;
    message.type = 0;
    message.dataLength = 0;
    //message.checksum = 0;
    //message.timestamp = NULL;
    message.data = NULL;


    // get the type
    uint8_t type;
    numBytes = read(clientFD, &type, 1);
    if (numBytes < 1) {
        printError("Error reading the type\n");
    }
    // asprintf(&buffer, "Type: 0x%02x\n", type);
    // printToConsole(buffer);
    // free(buffer);
    message.type = type;

    // get the data length
    uint16_t dataLength;
    numBytes = read(clientFD, &dataLength, sizeof(unsigned short));
    if (numBytes < (ssize_t)sizeof(unsigned short)) {
        printError("Error reading the Data Length\n");
    }
    // asprintf(&buffer, "Data length: %u\n", dataLength);
    // printToConsole(buffer);
    // free(buffer);
    message.dataLength = dataLength;

    // get the checksum
    //message.checksum = calculateChecksum(message.data, numBytes);


    // get the timestamp
    //message.timestamp = getCurrentTimestamp();

    // get the data
    char *data = malloc(sizeof(char) * 256 - 9 - dataLength);
    numBytes = read(clientFD, data, 256 - 9 - dataLength);

    if (numBytes < 256 - 9 - dataLength) {
        printError("Error reading the data\n");
        free(data);
    } else {
        message.data = malloc(sizeof(char) * numBytes);
        memcpy(message.data, data, numBytes);
    }
    /*asprintf(&buffer, "Data: %s\n", data);
    printToConsole(buffer);
    free(buffer);*/

    // message.data = strdup(data);

    free(data);

    return message;
}




