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

#define perror2(s, e) fprintf(stderr, "%s: %s\n", strerror(e));

FILE *fdPollStats, *fdPollLog;
Queue bufferQueue;
pthread_t *tids;
pthread_mutex_t mtx;
pthread_mutex_t writeMtx;
pthread_cond_t cond_nonempty;
pthread_cond_t cond_nonfull;
int numWorkerThreads;
bool sigintFlag=false;
static sigset_t signal_mask;  /* signals to block         */

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
    printf ( " Thread % ld : Sent signal \n " , pthread_self() );
    pthread_mutex_unlock(&mtx);
    

    // printf("Im here\n");
    // fclose(fdPollLog); fclose(fdPollStats);
    // printf("Im here\n");
    // queue_destroy(bufferQueue);
    // printf("Im here\n");
    // pthread_cond_destroy(&cond_nonempty);
    // pthread_cond_destroy(&cond_nonfull);
    // pthread_mutex_destroy(&mtx);
    // printf("Im here\n");

    
    printf("IM HERE\n");
    // exit(EXIT_SUCCESS);
}

void assignHandler() {
    static struct sigaction act;
    act.sa_handler=catchInterupt;
    sigfillset(&(act.sa_mask));

    sigaction(SIGINT, &act, NULL);
}

int main(int argc, char **argv) {
    int portnum, bufferSize;
    char *pollLogFile, *pollStatsFile;
    int sock , newsock, err;
    struct sockaddr_in server , client ;
    socklen_t clientlen=sizeof(client); ;
    struct sockaddr * serverptr =( struct sockaddr *) & server ;
    struct sockaddr * clientptr =( struct sockaddr *) & client ;
    
    assignHandler();

    if(checkArgumentsServer(argc, argv, &portnum, &numWorkerThreads, \
        &bufferSize, &pollLogFile, &pollStatsFile) == false)
        exit(EXIT_FAILURE);

    // Initialise the buffer queue, the destroy value will
    // be free because queue will store int *value allocated in the heap
    bufferQueue = queue_create(free, bufferSize);
    
    // Open the files
    // if((fdPollLog = open(pollLogFile, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)   
    //     perror_exit("File opening");
    // printf("The file is %s\n", pollLogFile);
    if((fdPollLog = fopen(pollLogFile, "a")) == NULL)
        perror_exit("File opening");
    if((fdPollStats = fopen(pollStatsFile, "a")) == NULL)
        perror_exit("File opening");

    // Allocate the array of threads
    if((tids=malloc(numWorkerThreads*sizeof(pthread_t)))==NULL)
        perror_exit("malloc");

    int        rc;              /* return code              */


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

    printf ( "Im thread %ld \n " , pthread_self() );
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
            printf("new sokc is %d\n",errno);
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
        printf("THE NEWSOCK is %d\n",newsock);
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

    // int err;
    printf("num is %d\n", numWorkerThreads);
    for(int i=0 ; i<numWorkerThreads ; i++) {
        printf("Im here to wait for %d\n", i);
        if(err=pthread_join(tids[i],NULL)) {
            perror2("pthread_join", err);
            exit(EXIT_FAILURE);
        } 
        printf("came here");
    }
    free(tids);

     printf("Im here\n");
    fclose(fdPollLog); fclose(fdPollStats);
    printf("Im here\n");
    queue_destroy(bufferQueue);
    printf("Im here\n");
    pthread_cond_destroy(&cond_nonempty);
    pthread_cond_destroy(&cond_nonfull);
    pthread_mutex_destroy(&mtx);
    pthread_mutex_destroy(&writeMtx);
    printf("Im here\n");

    return 0;
}

void child_server(int newsock) {
    char sendNameBuff[18]="SEND NAME PLEASE\n";
    char sendPartyBuff[18]="SEND VOTE PLEASE\n";
    char nameSurname[64]="";
    char party[64]="";
    if(write(newsock, sendNameBuff, 18) < 0) {
        perror_exit("write");
    }
    if(read(newsock, nameSurname, 64) > 0) {  /* Receive 1 char */
    	// putchar(buf[0]);           /* Print received char */
        printf("Name is %s\n",nameSurname);
    	/* Capitalize character */
    	// buf[0] = toupper(buf[0]);
    	/* Reply */
    	// if (write(newsock, buf, 1) < 0)
    	//     perror_exit("write");
    }
    if(write(newsock, sendPartyBuff, 18) < 0) {
        perror_exit("write");
    }
    if(read(newsock, party, 64) > 0) {  /* Receive 1 char */
    	// putchar(buf[0]);           /* Print received char */
        printf("Vote is %s\n",party);
    	/* Capitalize character */
    	// buf[0] = toupper(buf[0]);
    	/* Reply */
    	// if (write(newsock, buf, 1) < 0)
    	//     perror_exit("write");
    }
    printf("Closing connection.\n");
    close(newsock);	  /* Close socket */
}

void *worker_thread(void *value) { 
    printf ( "Im thread %ld \n " , pthread_self() );
    int socketDes;
    printf("Im in the working thread\n");
    pthread_mutex_lock(&mtx);
    while(queue_empty(bufferQueue)) {
        printf("Buffer is empty\n");
        pthread_cond_wait(&cond_nonempty, &mtx);
        printf("EIMAI ANAMESA\n");
        if(sigintFlag==true) {
            printf("The flag changed\n");
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
    char space[2] = " ";
    char newLine[2] = "\n";
    char nameSurname[64]="";
    char party[64]="";
    if(write(socketDes, sendNameBuff, 18) < 0) {
        perror_exit("write");
    }
    if(read(socketDes, nameSurname, 64) > 0) {  /* Receive 1 char */
        printf("Name is %s\n",nameSurname);
        // for (ssize_t i = 0; i < strlen(nameSurname)+1; i++) {
        //     if (nameSurname[i] == '\n') {
        //         // Reached the end of the line
        //         nameSurname[i] = '\0'; // Null-terminate the string if needed
        //     }
        // }   
    	// putchar(buf[0]);           /* Print received char */
        printf("Name is %s with %d\n",nameSurname, strlen(nameSurname)-1);

        // Before writing to the file first lock the mutex
        pthread_mutex_lock(&writeMtx);
        fwrite(nameSurname, strlen(nameSurname)-2, 1, fdPollLog);
        fwrite(space, strlen(space), 1, fdPollLog);
    	/* Capitalize character */
    	// buf[0] = toupper(buf[0]);
    	/* Reply */
    	// if (write(newsock, buf, 1) < 0)
    	//     perror_exit("write");
    }
    if(write(socketDes, sendPartyBuff, 18) < 0) {
        perror_exit("write");
    }
    if(read(socketDes, party, 64) > 0) {  /* Receive 1 char */
    	// putchar(buf[0]);           /* Print received char */
        printf("Vote is %s\n",party);
        fwrite(party, strlen(party)-2, 1, fdPollLog);
        fwrite(newLine, strlen(newLine), 1, fdPollLog);
        pthread_mutex_unlock(&writeMtx);

        // Initialise a string that will get inserted in the map
        // so that we can print the poll stats
        // char *partyTBInerted;
        // memcpy(partyTBInerted, party, strlen(party)-1);
        // fprintf(stderr,"THE PARTY TO BE INSERTED IS %s OOOOO\n", partyTBInerted);
        party[strlen(party) - 2] = '\0';
        printf("The stirng is %s\n", party);
    	/* Capitalize character */
    	// buf[0] = toupper(buf[0]);
    	/* Reply */
    	// if (write(newsock, buf, 1) < 0)
    	//     perror_exit("write");
    }
    printf("Closing connection.\n");
    close(socketDes);	  /* Close socket */

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