#pragma once // #include once

#include <stdbool.h>

// Check if the arguments are in the correct format
bool checkArgumentsServer(int argc, char **argv, int *portnum, \
int *numWorkerThreads, int *bufferSize, char **pollLogFile, char **pollStatsFile);

bool checkArgumentsClient(int argc, char **argv, char **serverName, int *portNum, char **inputFile);

bool readSocket(int socketDes, char *str);

void perror_exit(char *message);

void get_name_party(char *str, char *name, char *party);