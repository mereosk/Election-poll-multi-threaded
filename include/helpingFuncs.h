#pragma once // #include once

#include <stdbool.h>

// Check if the arguments of server are in the correct format
bool check_arguments_server(int argc, char **argv, int *portnum, \
int *numWorkerThreads, int *bufferSize, char **pollLogFile, char **pollStatsFile);

// Check if the arguments of server are in the correct format
bool check_arguments_client(int argc, char **argv, char **serverName, int *portNum, char **inputFile);

// Reads a socket up to 64 characters, if there are more characters 
// then returns false orelse it returns true
bool read_socket(int socketDes, char *str);

void perror_exit(char *message);

// Takes a string 'str' that has the format:
// NAME SURNAME PARTY
// it returns the NAME SURNAME via 'name' arg and the PARTY via
// 'party' arg. It returns them with \n in the end
void get_name_party(char *str, char *name, char *party);