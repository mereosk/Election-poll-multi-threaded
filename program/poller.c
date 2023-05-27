#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "helpingFuncs.h"


int main(int argc, char **argv) {
    int portnum, numWorkerThreads, bufferSize;

    if(checkArguments(argc, argv, &portnum, &numWorkerThreads, &bufferSize) == false)
        exit(EXIT_FAILURE);

    printf("Im here\n");
    

    return 0;
}