#include "utils_connect.h"
#include "io_utils.h"
#include <stdio.h>
#include "../struct_definitions.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int getFileSize(char *fileName)
{

    FILE *file = fopen(fileName, "rb");
    if (file == NULL)
    {
        printError("Error al abrir el archivo\n");
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

        // Leer la salida del proceso hijo
        char buffer[256];
        int bytesRead = read(pipefd[0], buffer, sizeof(buffer));
        if (bytesRead == -1)
        {
            perror("Error al leer del pipe");
            return NULL;
        }

        // Eliminar el salto de línea al final de la cadena
        buffer[bytesRead - 1] = '\0';

        // Almacenar solo el valor de MD5 en una cadena
        char *md5sum_start = strchr(buffer, ' ');
        if (md5sum_start != NULL)
        {
            *md5sum_start = '\0';
        }
        md5sum = strdup(buffer);

        close(pipefd[0]); // Cerrar el extremo de lectura del pipe

        // Esperar a que termine el proceso hijo
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {

            free(md5sum);
            printError("El comando md5sum falló\n");
            return NULL;
        }
    }

    return md5sum;
}