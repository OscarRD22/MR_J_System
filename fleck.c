#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "struct_definitions.h"
#include "utils/io_utils.h"
#include "utils/utils_connect.h"
#include "utils/utils_fleck.h"

// Osar.romero - Marc.marza
// Variables globales para el progreso de distorsión
typedef struct
{
    char filename[256];
    int progress; // Porcentaje completado (0-100)
} DistortionProgress;

extern int gothamSocketFD, distorsionSocketFD;
// This is the client
Fleck fleck;
char *command;
int iSConnected = FALSE;
int distortionCount = 0;
DistortionProgress distortions[10]; // Máximo 10 distorsiones simultáneas
pthread_t DistorsionThreadTxt, DistorsionThreadMedia;

/**
 * @brief Saves the information of the Fleck file into the Fleck struct
 *  @param filename The name of the file to read
 */
void saveFleck(char *filename)
{
    int data_file_fd = open(filename, O_RDONLY);
    if (data_file_fd < 0)
    {
        printError("Error while opening the Fleck file");
        exit(1);
    }
    fleck.username = readUntil('\n', data_file_fd);
    validateString(fleck.username);
    fleck.folder = readUntil('\n', data_file_fd);
    fleck.ip = readUntil('\n', data_file_fd);
    char *port = readUntil('\n', data_file_fd);
    fleck.port = atoi(port);
    free(port);

    char *buffer;
    asprintf(&buffer, "%s user has been initialized\n$ ", fleck.username);
    printToConsole(buffer);
    free(buffer);
    close(data_file_fd);
}
/**
 * @brief Free the memory allocated
 */
void freeMemory()
{
    free(fleck.username);
    free(fleck.folder);
    free(fleck.ip);
    if (command != NULL)
    {
        free(command);
    }
}

void closeFds()
{
    if (gothamSocketFD != 0)
    {
        close(gothamSocketFD);
    }
    if (distorsionSocketFD != 0)
    {
        close(distorsionSocketFD);
    }
}

/**
 * @brief Closes the file descriptors if they are open
 */
void closeProgram()
{
    freeMemory();
    closeFds();
    printToConsole("Closing program Fleck\n");
    exit(0);
}
/**
 * @brief Closes the program correctly cleaning the memory and closing the file descriptors
 */
void closeProgramSignal()
{
    freeMemory();
    closeFds();
    exit(0);
}

/**
 * @brief Inital setup of the program
 * @param argc The number of arguments
 */
void initalSetup(int argc)
{
    if (argc != 2)
    {
        printError("ERROR: Not enough arguments provided\n");
        exit(1);
    }
    signal(SIGINT, closeProgramSignal);
}

/**
 * @brief Limpia todos los recursos del cliente y desconecta de Gotham.
 */
void clearAll()
{
    if (iSConnected)
    {
        printToConsole("Disconnecting from Mr. J System...\n");
        connectToGotham(TRUE);
        iSConnected = FALSE;
    }

    if (command)
    {
        free(command);
        command = NULL;
    }

    closeFds();
    freeMemory();

    printToConsole("All resources have been cleared.\n");
}

//!----------------------------------------------------- M A I N - L O G I C ------------------------------------------------------------------
void commandInterpretter()
{
    int continueReading = TRUE;

    do
    {
        if (command != NULL)
        {
            free(command);
            command = NULL;
        }
        command = readUntil('\n', 0);

        if (strcasecmp(command, "CONNECT") == 0)
        {
            if (iSConnected)
            {
                printError("You are already connected to Mr. J System\n");
            }
            else
            {
                int result = connectToGotham(FALSE);
                if (result == 0)
                {
                    printToConsole("Connected to Mr. J System. Let the chaos begin! :)\n");
                    iSConnected = TRUE;
                }
                else
                {
                    printError("Failed to connect to Gotham.\n");
                }
            }
        }
        else if (strcasecmp(command, "LOGOUT") == 0)
        {
            if (iSConnected)
            {
                printToConsole("Disconnecting from Mr. J System...\n");
                connectToGotham(TRUE); // Envía mensaje de desconexión
                // pthread_join(DistorsionThread, NULL);

                iSConnected = FALSE;
            }
            printToConsole("Thanks for using Mr. J System, see you soon, chaos lover :)\n");
            continueReading = FALSE;
        } //! COMMAND HAS MORE THAN ONE WORD----------------------------------------------------------------
        else if (strncmp(command, "DISTORT ", 8) == 0)
        {
            if (!iSConnected)
            {
                printError("Cannot distort, you are not connected to Mr. J System\n");
            }
            else
            {
                char *filename = strtok(&command[8], " ");
                char *factor = strtok(NULL, " ");
                char *path = ("./files/");
                char *fullPath;
                asprintf(&fullPath, "%s%s", path, filename);
                if (filename == NULL || factor == NULL)
                {
                    printError("Invalid DISTORT command. Usage: DISTORT <filename> <factor>\n");
                }

                else
                {
                    char *extension = strrchr(fullPath, '.');
                    if (!extension || strlen(extension) < 2)
                    {
                        printError("Invalid file type. Please provide a valid file.\n");
                        return;
                    }

                    // Treure l'extensió (sense el punt inicial)
                    char *mediaType = extension + 1;

                    ThreadDistortionParams *params = malloc(sizeof(ThreadDistortionParams));
                    // Assignar memòria dinàmica per a fullPath, filename i factor
                    params->fullPath = malloc(strlen(fullPath) + 1);
                    strcpy(params->fullPath, fullPath);

                    params->filename = malloc(strlen(filename) + 1);
                    strcpy(params->filename, filename);

                    params->factor = malloc(strlen(factor) + 1);
                    strcpy(params->factor, factor);

                    printf("Media type: %s - %s - %s\n", params->fullPath, params->filename, params->factor);

                    if (strcasecmp(mediaType, "txt") == 0)
                    {
                        if (isTxtDistortRunning)
                        {
                            printError("A text distortion is already in progress. Please wait for it to finish.\n");
                        }
                        else
                        {
                            if (pthread_create(&DistorsionThreadTxt, NULL, (void *)handleDistortCommand, (void *)params) != 0)
                            {
                                printError("Error creating Text thread\n");
                            }
                            else
                            {
                                isTxtDistortRunning = TRUE;
                            }
                        }
                    }
                    else
                    {
                        if (isMediaDistortRunning)
                        {
                            printError("A media distortion is already in progress. Please wait for it to finish.\n");
                        }
                        else
                        {
                            if (pthread_create(&DistorsionThreadMedia, NULL, (void *)handleDistortCommand, (void *)params) != 0)
                            {
                                printError("Error creating Media thread \n");
                            }
                            else
                            {
                                isMediaDistortRunning = TRUE;
                            }
                        }
                    }
                    // char *b;
                    // asprintf(&b, "DISTORT  FILENAME(%s) FACTOR(%s)\n", filename, factor);
                    // printToConsole(b);
                    // free(b);

                    // handleDistortCommand(fullPath, filename, factor);
                }
            }
        }
        else if (strncasecmp(command, "CHECK ", 6) == 0)
        {
            char *subCommand = strtok(&command[6], " ");
            if (subCommand == NULL)
            {
                printError("ERROR: Please input a valid CHECK command.\n");
            }
            else if (strcasecmp(subCommand, "STATUS") == 0)
            {
                // checkStatus();
            }
            else
            {
                printError("Unknown CHECK command.\n");
            }
        }
        else if (strcasecmp(command, "LIST MEDIA") == 0)
        {
            listMedia();
        }
        else if (strcasecmp(command, "LIST TEXT") == 0)
        {
            listText();
        }
        else if (strcasecmp(command, "CLEAR ALL") == 0)
        {
            clearAll();
        }
        else
        {
            printError("Unknown command\n");
        }

        // Muestra el prompt después de cada comando
        printToConsole("$ ");
    } while (continueReading);

    if (command != NULL)
    {
        free(command);
        command = NULL;
    }
}

/**
 * @brief Main function
 * @param argc The number of arguments
 * @param argv The arguments
 * @return int The exit status
 */
int main(int argc, char *argv[])
{
    initalSetup(argc);
    saveFleck(argv[1]);
    commandInterpretter();
    closeProgram();
    return 0;
}