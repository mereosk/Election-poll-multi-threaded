#include <stdio.h>
#include <stdlib.h>

#include "helpingFuncs.h"

int main(int argc, char **argv) {
    char *serverName, *inputFile;
    int portNum;

    printf("Im in the poll swayer\n");

    if(checkArgumentsClient(argc, argv, serverName, &portNum, inputFile) == false)
        exit(EXIT_FAILURE);

    return 0;
}