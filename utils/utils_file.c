#include "utils_connect.h"
#include "io_utils.h"
#include <stdio.h>
#include "../struct_definitions.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int getFileSize(char *fileName)
{
    FILE *file = fopen(fileName, "rb");
    if (file == NULL)
    {
        printError("Error al abrir el archivo");
        return -1;
    }
    // Mover el cursor al final del archivo para determinar su tamaño
    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file); // Obtener el tamaño del archivo
    fclose(file);

    return fileSize;
}

char *calculateMD5(char *filePath)
{
    int pipefd[2];
    pid_t pid;
    char *md5sum = NULL;

    // Crear un pipe para la comunicación entre padre e hijo
    if (pipe(pipefd) == -1)
    {
        perror("Error al crear el pipe");
        return NULL;
    }

    // Crear un proceso hijo
    pid = fork();
    if (pid < 0)
    {
        perror("Error al crear el proceso hijo");
        return NULL;
    }
    else if (pid == 0)
    {
        // Código del proceso hijo
        close(pipefd[0]); // Cerrar el extremo de lectura del pipe

        // Redirigir la salida estándar al extremo de escritura del pipe
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]); // Cerrar el descriptor de escritura original

        // Ejecutar el comando md5sum
        execlp("md5sum", "md5sum", filePath, NULL);

        // Si execlp falla
        printError("Error al ejecutar md5sum");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Código del proceso padre
        close(pipefd[1]); // Cerrar el extremo de escritura del pipe

        int bufferSize = 32 + 1 + strlen(filePath)+1;
        // Leer el resultado del pipe
        char buffer[bufferSize];
        ssize_t bytesRead = read(pipefd[0], buffer, bufferSize -1);
        if (bytesRead == bufferSize - 1)
        {
            buffer[bytesRead] = '\0';     // Asegurar que el buffer es un string
            memcpy(md5sum, buffer, 32); // Copiar los primeros 32 caracteres (MD5SUM)
        }
        else
        {
            perror("Error al leer desde el pipe");
        }
        close(pipefd[0]); // Cerrar el extremo de lectura del pipe

        // Esperar a que termine el proceso hijo
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            fprintf(stderr, "El comando md5sum falló con código %d\n", WEXITSTATUS(status));
            free(md5sum);
            return NULL;
        }
    }

    return md5sum;
}