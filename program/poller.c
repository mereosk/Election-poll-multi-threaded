#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>	     /* sockets */
#include <sys/types.h>	     /* sockets */
#include <sys/socket.h>	     /* sockets */
#include <netinet/in.h>	     /* internet sockets */
#include <netdb.h>	         /* gethostbyaddr */
#include <unistd.h>	         /* fork */		
#include <ctype.h>	         /* toupper */
#include <signal.h>          /* signal */
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "helpingFuncs.h"
#include "ADTQueue.h"
#include "ADTMap.h"

#define perror2(s, e) fprintf(stderr, "%s: %s\n", strerror(e));

FILE *fdPollStats, *fdPollLog;
Queue bufferQueue;
pthread_t *tids;
pthread_mutex_t mtx;
pthread_mutex_t writeMtx;
pthread_mutex_t statsMapMtx;
pthread_mutex_t checkMapMtx;
pthread_cond_t cond_nonempty;
pthread_cond_t cond_nonfull;
int numWorkerThreads;
bool sigintFlag=false;
static sigset_t signal_mask;  // signals to block
Map statsMap;
Map checkMap;
int totalVotes = 0;

int *create_int(int value) {
    int *ret = malloc(sizeof(int));
    *ret=value;
    return ret;
}

void child_server(int newsock);
void perror_exit(char *message);
void sigchld_handler (int sig);
void *worker_thread(void *value);


void catchInterupt(int signo) {
    pthread_mutex_lock(&mtx);
    sigintFlag=true;
    // pthread_cond_broadcast(&cond_nonfull);
    pthread_cond_broadcast(&cond_nonempty);
    pthread_mutex_unlock(&mtx);
}

void assignHandler() {
    static struct sigaction act;
    act.sa_handler=catchInterupt;
    sigfillset(&(act.sa_mask));

    sigaction(SIGINT, &act, NULL);
}

int str_compare(Pointer a,Pointer b){
    return strcmp(a, b);
}

int main(int argc, char **argv) {
    int portnum, bufferSize;
    char *pollLogFile, *pollStatsFile;
    int sock , newsock, err;
    struct sockaddr_in server , client ;
    socklen_t clientlen=sizeof(client); ;
    struct sockaddr * serverptr =( struct sockaddr *) & server ;
    struct sockaddr * clientptr =( struct sockaddr *) & client ;

    // Initialise the statsMap. This map will save the stats in order
    // to write them in the poll-stats.txt after SIGINT. This map
    // will have the party name as the key and a number as the value
    // that will give us the # of votes for that specific party
    statsMap = map_create(str_compare, free, free);
    // As for the hash function of the map we will use the djb2 hash function
    map_set_hash_function(statsMap, hash_string);

    checkMap = map_create(str_compare, free, NULL);
    map_set_hash_function(checkMap, hash_string);
    
    assignHandler();

    if(checkArgumentsServer(argc, argv, &portnum, &numWorkerThreads, \
        &bufferSize, &pollLogFile, &pollStatsFile) == false)
        exit(EXIT_FAILURE);

    // Initialise the buffer queue, the destroy value will
    // be free because queue will store int *value allocated in the heap
    bufferQueue = queue_create(free, bufferSize);
    
    if((fdPollLog = fopen(pollLogFile, "w")) == NULL)
        perror_exit("File opening");
    if((fdPollStats = fopen(pollStatsFile, "w")) == NULL)
        perror_exit("File opening");

    // Allocate the array of threads
    if((tids=malloc(numWorkerThreads*sizeof(pthread_t)))==NULL)
        perror_exit("malloc");

    int rc;     // return code

    sigemptyset (&signal_mask);
    sigaddset (&signal_mask, SIGINT);
    rc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
    if (rc != 0) {
        fprintf( stderr,"error in mask");
        exit(EXIT_FAILURE);
    }

    // Create all the threads
    for(int j=0 ; j<numWorkerThreads ; j++) {
        if(err = pthread_create(tids+j, NULL, worker_thread, NULL)) {
            perror2("pthread_create", err);
            exit(EXIT_FAILURE);
        }
    }

    // Unblock SIGINT for the main thread
    sigset_t unblock_set;
    sigemptyset(&unblock_set);
    sigaddset(&unblock_set, SIGINT);
    pthread_sigmask(SIG_UNBLOCK, &unblock_set, NULL);

    // Reap dead children asynchronously
    // signal(SIGCHLD, sigchld_handler);
    /* Create socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror_exit("socket");
    server.sin_family = AF_INET;       /* Internet domain */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(portnum);      /* The given portnum */
    /* Bind socket to address */
    if (bind(sock, serverptr, sizeof(server)) < 0)
        perror_exit("bind");
    /* Listen for connections */
    if (listen(sock, 128) < 0) perror_exit("listen");
    printf("Listening for connections to port %d\n", portnum);
    while (1) {
        // Master thread accepts connection
    	if ((newsock = accept(sock, (struct sockaddr *)clientptr, &clientlen)) < 0) {
            if(errno==EINTR)
                break;
            else
                perror_exit("accept");
        }
        // Insert the sock descriptor in the buffer queue
        pthread_mutex_lock(&mtx);
        while(queue_full(bufferQueue)) {
            printf("The buffer is full");
            pthread_cond_wait(&cond_nonfull, &mtx);
        }
        queue_insert_back(bufferQueue, create_int(newsock));
        pthread_cond_signal(&cond_nonempty);
        pthread_mutex_unlock(&mtx);

    	/* Find client's address */
    	printf("Accepted connection\n");
    	// switch (fork()) {    /* Create child for serving client */
    	// case -1:     /* Error */
    	//     perror("fork"); break;
    	// case 0:	     /* Child process */
    	//     close(sock); 
        //     child_server(newsock);
    	//     exit(0);
    	// }
    	// close(newsock); /* parent closes socket to client */
    }

    for(int i=0 ; i<numWorkerThreads ; i++) {
        if(err=pthread_join(tids[i],NULL)) {
            perror2("pthread_join", err);
            exit(EXIT_FAILURE);
        } 
    }
    free(tids);

    // Write in the poll-stats.txt the data from the map
    map_insert_to_file(statsMap, fdPollStats, totalVotes);

    fclose(fdPollLog); fclose(fdPollStats);
    queue_destroy(bufferQueue);
    map_destroy(statsMap);
    map_destroy(checkMap);
    pthread_cond_destroy(&cond_nonempty);
    pthread_cond_destroy(&cond_nonfull);
    pthread_mutex_destroy(&mtx);
    pthread_mutex_destroy(&statsMapMtx);
    pthread_mutex_destroy(&checkMapMtx);
    pthread_mutex_destroy(&writeMtx);

    return 0;
}

void *worker_thread(void *value) { 
    while(1) {
printf ( "Im thread %ld \n " , pthread_self() );
    int socketDes;
    pthread_mutex_lock(&mtx);
    while(queue_empty(bufferQueue)) {
        printf("Buffer is empty\n");
        pthread_cond_wait(&cond_nonempty, &mtx);
        if(sigintFlag==true) {
            pthread_mutex_unlock(&mtx);
            return NULL;
        }
    }
    
    // Get the socket descriptor
    socketDes=*(int *)queue_front(bufferQueue);
    printf("The socket descriptor is %d\n", socketDes);
    queue_remove_front(bufferQueue);
    pthread_mutex_unlock(&mtx);

    pthread_cond_signal(&cond_nonfull);

    char sendNameBuff[18]="SEND NAME PLEASE\n";
    char sendPartyBuff[18]="SEND VOTE PLEASE\n";
    char sendAlrVoted[15]="ALREADY VOTED\n";
    char space[2] = " ";
    char newLine[2] = "\n";
    char nameSurname[64]="";
    char party[128]="";
    char *line = calloc(256, sizeof(char));
    if(write(socketDes, sendNameBuff, 18) < 0) {
        perror_exit("write");
    }
    // while(read(socketDes, nameSurname, 32) > 0) { 
    //     if(strstr(nameSurname, newLine)) {
    //         break;
    //     }
    // }
    readSocket(socketDes, nameSurname);
    printf("Name is %s with %d\n",nameSurname, strlen(nameSurname)-1);
    nameSurname[strlen(nameSurname)-1] = '\0';

    // Check if the name is in the checkMap
    pthread_mutex_lock(&checkMapMtx);
    if(map_find(checkMap, nameSurname)!=NULL) {
        // That means that the name is a duplicate

        // Send the message "ALREADY VOTED"
        if(write(socketDes, sendAlrVoted, 15) < 0) {
            perror_exit("write");
        }
        // Close the connection
        free(line);
        printf("Closing connection.\n");
        close(socketDes);	  /* Close socket */

        pthread_mutex_unlock(&checkMapMtx);
        continue;
    }
    // Being here means that the name is not a duplicate
    // thus we have to insert it in the checkMap
    char *nameToBeInserted = strdup(nameSurname);
    char *valueToBeInserted = nameToBeInserted;
    map_insert(checkMap, nameToBeInserted, valueToBeInserted);
    pthread_mutex_unlock(&checkMapMtx);
        
    strncpy(line, nameSurname, strlen(nameSurname));
    strcat(line, space);
    if(write(socketDes, sendPartyBuff, 18) < 0) {
        perror_exit("write");
    }
    // while(read(socketDes, tempBuffer, 32) > 0) { 
    //     printf("%s OPA\n",tempBuffer);
    //     strcat(party, tempBuffer);
    //     if(strstr(party, newLine)) {
    //         break;
    //     }
    // }
    readSocket(socketDes, party);
    
    printf("Vote is %s\n",party);
    party[strlen(party)-1] = '\0';
    strcat(line, party);
    strcat(line, newLine);
    int *value;
    // Check if the key(party) is in the map
    pthread_mutex_lock(&statsMapMtx);
    // Increase the vote counter by 1, here is the perferct spot
    // because it is protected my a mutex
    totalVotes++;
    if((value=map_find(statsMap, party)) == NULL) {
        // Party is not in the map so insert it with that value 0
        map_insert(statsMap, strdup(party), create_int(1));
    }
    else {
        // Party is in the map so add 1 to the value
        *value=*value+1;
    }
    pthread_mutex_unlock(&statsMapMtx);
    // pthread_mutex_unlock(&writeMtx);

    pthread_mutex_lock(&writeMtx);
    fwrite(line, strlen(line), 1, fdPollLog);
    pthread_mutex_unlock(&writeMtx);
    // Now write the last message that will be sent to client
    char *lastMessage = calloc(256,sizeof(char));
    strcpy(lastMessage, "VOTE for Party ");
    strcat(lastMessage, party);
    strcat(lastMessage, " RECORDED\n");
    if(write(socketDes, lastMessage, 128) < 0) {
        perror_exit("write");
    }

    free(lastMessage);
    free(line);
    printf("Closing connection.\n");
    close(socketDes);	  /* Close socket */
    }
    
    return NULL;
}

/* Wait for all dead child processes */
void sigchld_handler (int sig) {
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}