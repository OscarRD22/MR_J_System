#define _GNU_SOURCE
#include <stdio.h>

#include "io_utils.h"
#include "../struct_definitions.h"


int getFileSize(char *fileName);
char *calculateMD5(char *filePath);
