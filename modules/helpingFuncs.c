#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "helpingFuncs.h"

bool checkArguments(int argc, char **argv, int *portnum, int *numWorkerThreads, int *bufferSize) {
    // Check that the arguments are 6, the correct format is:
    // ./poller [portnum] [numWorkerthreads] [bufferSize] [poll-log] [poll-stats]
    if( argc!=6 ) {
        fprintf(stderr, "\033[1;31mWrong format of arguments, should have been: \n  \
        ./poller [portnum] [numWorkerthreads] [bufferSize] [poll-log] [poll-stats]\033[0m\n");
        return false;
    }
    // Check if the first argument 'portnum' is an integer positive number
    if((*portnum = atoi(argv[1]))==0 || *portnum <= 0) {
        fprintf(stderr, "\033[1;31mFirst argument [portnum] should be a positive integer\033[0m\n");
        return false;
    }
    // Check if the second argument 'numWorkerThreads' is an integer positive number
    if((*numWorkerThreads = atoi(argv[2]))==0 || *numWorkerThreads <= 0) {
        fprintf(stderr, "\033[1;31mSecond argument [numWorkerThreads] should be a positive integer\033[0m\n");
        return false;
    }
    // Check if the first argument 'bufferSize' is an integer positive number
    if((*bufferSize = atoi(argv[3]))==0 || *bufferSize <= 0) {
        fprintf(stderr, "\033[1;31mThird argument [bufferSize] should be a positive integer\033[0m\n");
        return false;
    }

    // The arguments were correct
    return true;
}