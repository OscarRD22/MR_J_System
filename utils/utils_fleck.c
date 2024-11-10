#include "io_utils.h"
#include "utils_fleck.h"

#define MAX_FILES 100

void listMedia() {
    DIR *dir;
    struct dirent *ent;
    char *fileNames[MAX_FILES];
    int count = 0;

    dir = opendir("files");
    if (dir == NULL) {
        printError("Error opening directory\n");
        return;
    }

    // Recorrido: almacenar los nombres de archivos de medios en el array
    while ((ent = readdir(dir)) != NULL) {
        // Filtrar las entradas "." y ".." y excluir archivos ".txt"
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            if (!strstr(ent->d_name, ".txt")) {  // Excluir archivos de texto
                if (count < MAX_FILES) {  // Verificar el límite
                    fileNames[count] = strdup(ent->d_name);  // Almacenar el nombre
                    count++;
                }
            }
        }
    }
    closedir(dir);

    // Mostrar el encabezado si hay archivos de medios
    if (count > 0) {
        char *buffer = NULL;
        asprintf(&buffer, "There are %d media files available:\n", count);
        printToConsole(buffer);  // Imprimir el encabezado
        free(buffer);  // Liberar la memoria

        // Imprimir la lista de archivos
        for (int i = 0; i < count; i++) {
            asprintf(&buffer, "%d. %s\n", i + 1, fileNames[i]);
            printToConsole(buffer);
            free(buffer);  // Liberar la memoria de cada línea de impresión
            free(fileNames[i]);  // Liberar memoria de cada nombre de archivo
        }
    } else {
        printToConsole("No media files found\n");
    }
}

void listText() {
    DIR *dir;
    struct dirent *ent;
    char *fileNames[MAX_FILES];
    int count = 0;

    dir = opendir("files");
    if (dir == NULL) {
        printError("Error opening directory\n");
        return;
    }

    // Recorrido: almacenar los nombres de archivos de texto en el array
    while ((ent = readdir(dir)) != NULL) {
        // Filtrar las entradas "." y ".." y excluir archivos no ".txt"
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            if (strstr(ent->d_name, ".txt")) {  // Incluir solo archivos .txt
                if (count < MAX_FILES) {  // Verificar el límite
                    fileNames[count] = strdup(ent->d_name);  // Almacenar el nombre
                    count++;
                }
            }
        }
    }
    closedir(dir);

    // Mostrar el encabezado si hay archivos de texto
    if (count > 0) {
        char *buffer = NULL;
        asprintf(&buffer, "There are %d text files available:\n", count);
        printToConsole(buffer);  // Imprimir el encabezado
        free(buffer);  // Liberar la memoria

        // Imprimir la lista de archivos de texto
        for (int i = 0; i < count; i++) {
            asprintf(&buffer, "%d. %s\n", i + 1, fileNames[i]);
            printToConsole(buffer);
            free(buffer);  // Liberar la memoria de cada línea de impresión
            free(fileNames[i]);  // Liberar memoria de cada nombre de archivo
        }
    } else {
        printToConsole("No text files found\n");
    }
}