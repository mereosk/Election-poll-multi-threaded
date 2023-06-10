#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>	     /* sockets */
#include <sys/socket.h>	     /* sockets */
#include <netinet/in.h>	     /* internet sockets */
#include <unistd.h>          /* read, write, close */
#include <netdb.h>	         /* gethostbyaddr */
#include <string.h>	         /* strlen */
#include <pthread.h>

#define perror2(s, e) fprintf(stderr, "%s: %s\n", strerror(e));

#include "helpingFuncs.h"
#include "ADTVector.h"

char *serverName;
int portNum;
struct sockaddr *serverptr;
struct sockaddr_in server;

// This is the struct that will be passed as argument in a thread
struct threadInfo {
    pthread_t threadId;       // ID returned by pthread_create()
    char *line;               // From reading a line of inputFile
};

typedef struct threadInfo *ThreadInfo;

// This is the thread function
void *communicate_with_server(void *argp);
void freeThInfo(Pointer value);

int main(int argc, char **argv) {
    char *inputFile;
    FILE *fd;
    int err;
    serverptr = (struct sockaddr*)&server;
    char line[256];
    // The vector is used because we don't know the exact number of
    // lines from the inputFile. Each element of the vector will be threadInfo
    Vector threadsVec = vector_create(0, freeThInfo);
    ThreadInfo thInfo;

    // Check if the arguments are correct and are given in the correct order
    if(check_arguments_client(argc, argv, &serverName, &portNum, &inputFile) == false)
        exit(EXIT_FAILURE);

    
    // Open input file for reading
    if((fd = fopen(inputFile, "r"))==NULL) {
        perror_exit("File opening");
    }
    // Read the file line by line
    while (fgets(line, 256, fd)) {
        // If the line is empty skip it
        if(strcmp(line,"\n")==0)
            continue;
        
        thInfo = malloc(sizeof(struct threadInfo));
        thInfo->line = strdup(line);

        // Create the thread that will work on line
        if(err = pthread_create(&(thInfo->threadId), NULL, communicate_with_server, thInfo)) {
            perror2("pthread_create", err);
            exit(EXIT_FAILURE);
        }
        // Insert the thread info in the vector      
        vector_insert_last(threadsVec, thInfo);
    }
    int status;

    // Loop through the vector and use pthread join in order to 
    // wait for the threads termination
    for(VectorNode vnode=vector_first(threadsVec); 
        vnode!=VECTOR_EOF;
        vnode = vector_next(threadsVec, vnode)) {
            ThreadInfo tempThInfo = (ThreadInfo)vector_node_value(threadsVec, vnode);
            pthread_t tempTid = tempThInfo->threadId;
            if(err = pthread_join(tempTid, (void **) &status)) {
                perror2("pthread_join", err);
                exit(EXIT_FAILURE);
            }
    } 

    printf("All threads have terminated\n");
	
    vector_destroy(threadsVec);
    fclose(fd);
    
    return 0;
}

void *communicate_with_server(void *argp) {
    ThreadInfo info = argp;
    int sock;
    struct hostent *rem;
    char *name = calloc(strlen(info->line),sizeof(char));
    char *party = calloc(strlen(info->line),sizeof(char));
    char firstMessage[18] = "";
    char secondMessage[18] = "";
    char thirdMessage[128] = "";

    // Get the name surname and party from the line that was
    // given to the thread
    get_name_party(info->line, name, party);

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    	perror_exit("socket");
	// Find server address
    if ((rem = gethostbyname(serverName)) == NULL) {	
	   herror("gethostbyname"); exit(1);
    }
    server.sin_family = AF_INET;       // Internet domain
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(portNum);         // Server port
    // Initiate connection
    if (connect(sock, serverptr, sizeof(server)) < 0)
	   perror_exit("connect");
    
    // Read the first message sent by server
    read_socket(sock, firstMessage);
    // Check if it is 'SEND NAME PLEASE'
    if (strcmp(firstMessage, "SEND NAME PLEASE\n") == 0) {
        // Send the name and surname to server
        if(write(sock, name, strlen(name)) < 0) {
            perror_exit("write");
        }
    } 
    // Read the second message sent by server
    read_socket(sock, secondMessage);
    // Check if it is 'SEND VOTE PLEASE'
    if (strcmp(secondMessage, "SEND VOTE PLEASE\n") == 0) {
        // Send the vote to server
        if(write(sock, party, strlen(party)) < 0)
            perror_exit("write");
    }   // Check if it is 'ALREADY VOTED'
    else if (strcmp(secondMessage, "ALREADY VOTED\n") == 0) {
        // In which case this person has already voted, close the connection
        close(sock);
        free(name); free(party);
        return NULL;
    }
    else {
        // Coming here means that there was an error so exit the program
        close(sock);
        fprintf(stderr,"Wrong string sent by server");
        exit(EXIT_FAILURE);
    }

    // Read the third message that will be 'VOTE for Party XYZ RECORDED'
    read_socket(sock, thirdMessage);

    // Close the socket
    close(sock);
    free(name); free(party);
    return NULL;
}

// Gracefully free the threadInfo
void freeThInfo(Pointer value) {
    ThreadInfo valuee = (ThreadInfo)value;
    free(valuee->line);
    free(valuee);
}