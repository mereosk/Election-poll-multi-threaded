#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "helpingFuncs.h"

#define MAXSIZE 64


bool check_arguments_server(int argc, char **argv, int *portnum, int *numWorkerThreads, int *bufferSize, char **pollLogFile, char **pollStatsFile) {
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
    *pollLogFile = argv[4];
    *pollStatsFile = argv[5];

    // The arguments were correct
    return true;
}

bool check_arguments_client(int argc, char **argv, char **serverName, int *portNum, char **inputFile) {
    // Check that the arguments are 4, the correct format is:
    // ./pollSwayer [serverName] [portNum] [inputFile.txt]
    if( argc!=4 ) {
        fprintf(stderr, "\033[1;31mWrong format of arguments, should have been: \n  \
        ./pollSwayer [serverName] [portNum] [inputFile.txt]\033[0m\n");
        return false;
    }
    // Check if the first argument 'portnum' is an integer positive number
    if((*portNum = atoi(argv[2]))==0 || *portNum <= 0) {
        fprintf(stderr, "\033[1;31mFirst argument [portnum] should be a positive integer\033[0m\n");
        return false;
    }
    *serverName = argv[1];
    *inputFile = argv[3];

    // The arguments were correct
    return true;
}

bool read_socket(int socketDes, char *str) {
    char newLine[2]="\n";
    char tempBuffer[MAXSIZE]="";
    int bytes;
    while ((bytes = read(socketDes, tempBuffer, MAXSIZE - 1)) > 0) {
        tempBuffer[bytes] = '\0'; // Terminate the buffer with a null character

        if (strlen(str) + bytes < MAXSIZE) {
            strcat(str, tempBuffer);
        } 
        else {
            // Buffer overflow occurred, handle the error
            fprintf(stderr, "Buffer overflow occurred\n");
            return false;
        }

        if (strstr(str, newLine) != NULL) {
            // Newline found, exit the loop
            break;
        }
    }

    if (bytes == -1) {
        // Error occurred during read, handle the error
        perror("read");
        exit(EXIT_FAILURE);
    }

    return true;
}

void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void get_name_party(char *str, char *name, char *party) {
    int countSpaces=0;
    bool flag=false;
    int i,j=0;
    // Run through the string
    for(i=0 ; i<strlen(str) ; i++) {
        // If it encounters the second space that means that
        // the name surname part is over
        if(str[i] == ' ' && countSpaces==1) {
            flag=true;
            continue;
        }

        // If it encounters the first space continue to
        // take the surname too
        if(str[i] == ' ' && countSpaces==0)
            countSpaces++;
        
        // When flag is false insert the char to name
        // and when it is true insert it to party
        if(flag==false)
            name[i] = str[i];
        else 
            party[j++] = str[i];
    }
    // In the end of name put \n
    name[strlen(name)]='\n';
    // If it doesn't already have it put \n in the end of party
    if(party[j-1]!='\n')
        party[j]='\n';
}