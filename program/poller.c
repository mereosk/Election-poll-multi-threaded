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

// Below there are global variables that all the worker threads can see 
FILE *fdPollStats, *fdPollLog;
// The buffer is a queue data structure
Queue bufferQueue;
pthread_t *tids;
// Mutexes
pthread_mutex_t mtx;
pthread_mutex_t writeMtx;
pthread_mutex_t statsMapMtx;
pthread_mutex_t checkMapMtx;
// Condition variables
pthread_cond_t cond_nonempty;
pthread_cond_t cond_nonfull;

int numWorkerThreads;
bool sigintFlag=false;
static sigset_t signal_mask;
// Maps
Map statsMap;
Map checkMap;

int totalVotes = 0;

int *create_int(int value);
void *worker_thread(void *value);
void catchInterupt(int signo);
void assignHandler();

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

    // Initialise the checkMap. This will save the names of the people
    // that have voted. It does not accept duplicates though. We need it
    // to check if someone have already voted. It has the person's name
    // as its key and value. Only the key is dynamically allocated so
    // that it doesn't take too much space to the heap
    checkMap = map_create(str_compare, free, NULL);
    // Same as before the hash function is djb2
    map_set_hash_function(checkMap, hash_string);
    
    assignHandler();

    // Check if the arguments are correct and in the correct order
    if(checkArgumentsServer(argc, argv, &portnum, &numWorkerThreads, \
        &bufferSize, &pollLogFile, &pollStatsFile) == false)
        exit(EXIT_FAILURE);

    // Initialise the buffer queue, the destroy value will
    // be free because queue will store int *value allocated in the heap
    bufferQueue = queue_create(free, bufferSize);
    
    // Open and truncate the poll-log and poll-stats files
    if((fdPollLog = fopen(pollLogFile, "w")) == NULL)
        perror_exit("File opening");
    if((fdPollStats = fopen(pollStatsFile, "w")) == NULL)
        perror_exit("File opening");

    // Allocate the array of threads
    if((tids=malloc(numWorkerThreads*sizeof(pthread_t)))==NULL)
        perror_exit("malloc");

    // Return code
    int rc;     

    // Block the signal before all the worker threads are created
    // I do this so that only the master thread will be able to execute
    // the signal (after the thread_create) the signal will be unblocked
    // for the master trhread
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

    // Unblock SIGINT for the main thread which the master thread too
    sigset_t unblock_set;
    sigemptyset(&unblock_set);
    sigaddset(&unblock_set, SIGINT);
    pthread_sigmask(SIG_UNBLOCK, &unblock_set, NULL);

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror_exit("socket");
    server.sin_family = AF_INET;       // Internet domain
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(portnum);      // The given portnum
    // Bind socket to address
    if (bind(sock, serverptr, sizeof(server)) < 0)
        perror_exit("bind");
    // Listen for connections
    if (listen(sock, 128) < 0) 
        perror_exit("listen");
    printf("Listening for connections to port %d\n", portnum);
    while (1) {
        // Master thread accepts connection
    	if ((newsock = accept(sock, (struct sockaddr *)clientptr, &clientlen)) < 0) {
            // When the SIGINT will come the errno will be EINTR, that means that
            // we shall leave the infinite loop
            if(errno==EINTR)
                break;
            else
                perror_exit("accept");
        }
        // Insert the sock descriptor in the buffer queue
        // It is critical section and a mutex must be locked
        pthread_mutex_lock(&mtx);
        while(queue_full(bufferQueue)) {
            // When the queue is full wait for the workers finish
            // their job but with 'wait' it doesn't do busy waiting
            printf("The buffer is full\n");
            pthread_cond_wait(&cond_nonfull, &mtx);
        }
        // Insert the newsock in the queue and inform the workers who
        // may be waiting if the queue is empty that they have work
        // to do ^.^
        queue_insert_back(bufferQueue, create_int(newsock));
        pthread_cond_signal(&cond_nonempty);
        pthread_mutex_unlock(&mtx);

    }
    // We come here only after a SIGINT

    // Join in order to wait for all the worker threads to terminate
    for(int i=0 ; i<numWorkerThreads ; i++) {
        if(err=pthread_join(tids[i],NULL)) {
            perror2("pthread_join", err);
            exit(EXIT_FAILURE);
        } 
    }
    free(tids);

    // Write in the poll-stats.txt the data from the map
    map_insert_to_file(statsMap, fdPollStats, totalVotes);

    // Free all the allocated space
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
    // Keep working till a sigint is received
    while(1) {
        int socketDes;
        // It is critical section
        pthread_mutex_lock(&mtx);
        while(queue_empty(bufferQueue)) {
            // Wait when the buffer is empty for a signal from the master
            // thread, but in this way we don't do busy waiting
            printf("Buffer is empty\n");
            pthread_cond_wait(&cond_nonempty, &mtx);
            // When the sigintFlag is true that means that a sigint has
            // been received and the worker has to return
            if(sigintFlag==true) {
                pthread_mutex_unlock(&mtx);
                return NULL;
            }
        }
        
        // Get the socket descriptor from the queue
        socketDes=*(int *)queue_front(bufferQueue);
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
        // Line will have the format:
        // NAME SURNAME PARTY
        char *line = calloc(256, sizeof(char));

        // Send the message 'SEND NAME PLEASE' to client
        if(write(socketDes, sendNameBuff, 18) < 0) {
            perror_exit("write");
        }

        // Read the name surname from the socket
        readSocket(socketDes, nameSurname);
        // The last character of the name surname will be \n
        // replace it with \0
        nameSurname[strlen(nameSurname)-1] = '\0';

        // Check if the name is in the checkMap\ 
        // Because the check map is dynamically allocated, thus 
        // all the threads can see it lock a mutex to protect the 
        // data from mistakes of parallelism
        pthread_mutex_lock(&checkMapMtx);
        if(map_find(checkMap, nameSurname)!=NULL) {
            // That means that the name is a duplicate

            // Send the message "ALREADY VOTED" to client
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

        // Only dynamically allocate the key no need to do it to 
        // the value too (its the same thing)
        char *nameToBeInserted = strdup(nameSurname);
        char *valueToBeInserted = nameToBeInserted;
        map_insert(checkMap, nameToBeInserted, valueToBeInserted);
        pthread_mutex_unlock(&checkMapMtx);
        
        // Start building the line that will be written in the poll-log.txt
        strncpy(line, nameSurname, strlen(nameSurname));
        strcat(line, space);

        // Send the message "SEND VOTE PLEASE" to client
        if(write(socketDes, sendPartyBuff, 18) < 0) {
            perror_exit("write");
        }

        // Read the vote from the socket
        readSocket(socketDes, party);
        // The last character of the vote will be \n
        // replace it with \0
        party[strlen(party)-1] = '\0';

        strcat(line, party);
        strcat(line, newLine);

        int *value;
        // Check if the key(party) is in the statsMap
        // Lock the statsMapMtx
        pthread_mutex_lock(&statsMapMtx);
        // Increase the vote counter by 1, here is the perferct spot
        // because it is protected my a mutex
        totalVotes++;
        if((value=map_find(statsMap, party)) == NULL) {
            // Party is not in the map so insert it with that value 1
            map_insert(statsMap, strdup(party), create_int(1));
        }
        else {
            // Party is in the map so increase the value by 1
            *value=*value+1;
        }
        pthread_mutex_unlock(&statsMapMtx);

        // Write the line NAME SURNAME VOTE to the poll-log.txt
        // Of course lock a mutex so that the data will not be 
        // written simultaneously by many threads
        pthread_mutex_lock(&writeMtx);
        fwrite(line, strlen(line), 1, fdPollLog);
        pthread_mutex_unlock(&writeMtx);
        // Now write the last message that will be sent to client
        // VOTE for Party XYZ RECORDED
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
        close(socketDes);	  // Close socket
    }
    
    return NULL;
}

// Takes an integer as the argument and returns a pointer
// pointing to this int, dynamically allocated
int *create_int(int value) {
    int *ret = malloc(sizeof(int));
    *ret=value;
    return ret;
}

// Catching the SIGINT signal and signals all the threads that 
// they must terminate
void catchInterupt(int signo) {
    // Need to lock the mutex because the sigintFlag is seen globally
    pthread_mutex_lock(&mtx);
    sigintFlag=true;
    // After this broadcast all the threads that are busy wait will see
    // that the sigintFlag is true and they will terminate gracefully
    pthread_cond_broadcast(&cond_nonempty);
    pthread_mutex_unlock(&mtx);
}

void assignHandler() {
    static struct sigaction act;
    act.sa_handler=catchInterupt;
    sigfillset(&(act.sa_mask));

    sigaction(SIGINT, &act, NULL);
}