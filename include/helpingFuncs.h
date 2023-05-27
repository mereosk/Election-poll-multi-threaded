#pragma once // #include once

// Check if the arguments are in the correct format
bool checkArgumentsServer(int argc, char **argv, int *portnum, int *numWorkerThreads, int *bufferSize);

bool checkArgumentsClient(int argc, char **argv, char *serverName, int *portNum, char *inputFile);