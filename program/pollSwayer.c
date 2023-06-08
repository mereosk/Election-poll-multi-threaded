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
char buf[18];

void perror_exit(char *message);

struct threadInfo {
    pthread_t threadId;        // ID returned by pthread_create()
    char *line;      // From reading a line of inputFile
};

typedef struct threadInfo *ThreadInfo;

void get_name_party(char *str, char *name, char *party) {
    int countSpaces=0;
    bool flag=false;
    int i,j=0;
    for(i=0 ; i<strlen(str)-1 ; i++) {
        putchar(str[i]);
        if(str[i] == ' ' && countSpaces==1) {
            flag=true;
            continue;
        }

        if(str[i] == ' ' && countSpaces==0)
            countSpaces++;
        

        if(flag==false)
            name[i] = str[i];
        else 
            party[j++] = str[i];
    }
    name[strlen(name)]='\n';
}

// This is the thread function
void *communicate_with_server(void *argp) {
    ThreadInfo info = argp;
    int sock;
    struct hostent *rem;
    char *name = calloc(strlen(info->line),sizeof(char));
    char *party = calloc(strlen(info->line),sizeof(char));
    printf("Im the newly created thread %ld with the string %s\n", pthread_self(), info->line);
    get_name_party(info->line, name, party);
    printf("Whole line is %s, name is %s and party is %s\n", info->line, name, party);

    /* Create socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    	perror_exit("socket");
	/* Find server address */
    if ((rem = gethostbyname(serverName)) == NULL) {	
	   herror("gethostbyname"); exit(1);
    }
    server.sin_family = AF_INET;       /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(portNum);         /* Server port */
    /* Initiate connection */
    if (connect(sock, serverptr, sizeof(server)) < 0)
	   perror_exit("connect");
    // printf("Connecting to %s port %d\n", argv[1], portNum);
    if(read(sock, buf, sizeof(buf)) < 0)
        perror_exit("read");
    if (strcmp(buf, "SEND NAME PLEASE\n") == 0) {
        if(write(sock, name, strlen(name)) < 0) {
            perror_exit("write");
        }
    } 
    // memcpy(buf, sizeof(buf), 0);
    char buffer[128];
    if(read(sock, buffer, sizeof(buf)) < 0)
        perror_exit("read");
    if (strcmp(buffer, "SEND VOTE PLEASE\n") == 0) {
        if(write(sock, party, strlen(party)) < 0)
            perror_exit("write");
    }
    else if (strcmp(buffer, "ALREADY VOTED\n") == 0) {
        printf("Ending\n");
    }
    else {
        close(sock);
        printf("buffer is %s\n", buf);
        fprintf(stderr,"Wrong string sent by server");
        exit(EXIT_FAILURE);
    }
    close(sock);
    free(name); free(party);
    return NULL;
}

void freeThInfo(Pointer value) {
    ThreadInfo valuee = (ThreadInfo)value;
    free(valuee->line);
    free(valuee);
}

int main(int argc, char **argv) {
    char *inputFile;
    FILE *fd;
    int err;
    serverptr = (struct sockaddr*)&server;
    size_t len = 0;
    char line[256];
    pthread_t tids;
    Vector threadsVec = vector_create(0, freeThInfo);
    ThreadInfo thInfo;

    printf("Im in the poll swayer\n");

    if(checkArgumentsClient(argc, argv, &serverName, &portNum, &inputFile) == false)
        exit(EXIT_FAILURE);
    
    // if((tids = malloc(10*sizeof(pthread_t)))==NULL){
    //     perror("malloc");
    //     exit(EXIT_FAILURE);
    // }
    
    // Open input file
    if((fd = fopen(inputFile, "r"))==NULL) {
        perror_exit("File opening");
    }
    // Read the file line by line
    int i=0;
    while (fgets(line, 256, fd)) {
        printf("%s", line);
        
        thInfo = malloc(sizeof(struct threadInfo));
        thInfo->line = strdup(line);
        // join later in the process
        // Create the thread that will work on line
        if(err = pthread_create(&(thInfo->threadId), NULL, communicate_with_server, thInfo)) {
            perror2("pthread_create", err);
            exit(EXIT_FAILURE);
        }
        // printf("TIDS is %d",(int)tids);
        // Insert the tid in the threadVector so that we can 
        
        vector_insert_last(threadsVec, thInfo);
        i++;
    }
    int status;

    // for(int i=0 ; i<10 ; i++){
    //     if(err = pthread_join(*(tids+i), (void **) &status)) {
    //         perror2("pthread_join", err);
    //         exit(EXIT_FAILURE);
    //     }
    // }
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

    printf("all threads have terminated\n");

	
    vector_destroy(threadsVec);
    // free(tids);
    fclose(fd);
    
    return 0;
}

void perror_exit(char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}