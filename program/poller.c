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

#include "helpingFuncs.h"

void child_server(int newsock);
void perror_exit(char *message);
void sigchld_handler (int sig);

int main(int argc, char **argv) {
    int portnum, numWorkerThreads, bufferSize;
    char *pollLogFile, *pollStatsFile;
    int sock , newsock ;
    struct sockaddr_in server , client ;
    socklen_t clientlen ;
    struct sockaddr * serverptr =( struct sockaddr *) & server ;
    struct sockaddr * clientptr =( struct sockaddr *) & client ;

    if(checkArgumentsServer(argc, argv, &portnum, &numWorkerThreads, \
        &bufferSize, &pollLogFile, &pollStatsFile) == false)
        exit(EXIT_FAILURE);
    
    // Reap dead children asynchronously
    signal(SIGCHLD, sigchld_handler);
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
    if (listen(sock, 5) < 0) perror_exit("listen");
    printf("Listening for connections to port %d\n", portnum);
    while (1) {
        /* accept connection */
    	if ((newsock = accept(sock, clientptr, &clientlen)) < 0) perror_exit("accept");
    	/* Find client's address */
    	printf("Accepted connection\n");
    	switch (fork()) {    /* Create child for serving client */
    	case -1:     /* Error */
    	    perror("fork"); break;
    	case 0:	     /* Child process */
    	    close(sock); child_server(newsock);
    	    exit(0);
    	}
    	close(newsock); /* parent closes socket to client */
    }

    printf("Im here\n");
    

    return 0;
}

void child_server(int newsock) {
    char buf[1];
    while(read(newsock, buf, 1) > 0) {  /* Receive 1 char */
    	putchar(buf[0]);           /* Print received char */
    	/* Capitalize character */
    	buf[0] = toupper(buf[0]);
    	/* Reply */
    	if (write(newsock, buf, 1) < 0)
    	    perror_exit("write");
    }
    printf("Closing connection.\n");
    close(newsock);	  /* Close socket */
}

/* Wait for all dead child processes */
void sigchld_handler (int sig) {
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}