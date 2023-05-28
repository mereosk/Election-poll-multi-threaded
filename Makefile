# paths
INCLUDE = ./include
MODULES = ./modules
PROGRAM = ./program
# compiler
CC = gcc

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CFLAGS = -Wall  -g -I$(INCLUDE)
LDFLAGS = 
ARGSSERVER = 5634 2 1 poll-log.txt poll-stats.txt
ARGSCLIENT = mereosk-Latitude-E7440 5634 inputFile.txt

# Αρχεία .o
OBJSSERVER = $(PROGRAM)/poller.o $(MODULES)/ADTList.o $(MODULES)/ADTVector.o $(MODULES)/ADTMap.o $(MODULES)/helpingFuncs.o
OBJSCLIENT = $(PROGRAM)/pollSwayer.o $(MODULES)/helpingFuncs.o

# Τα εκτελέσιμα πρόγραμματα
EXECSERVER = $(PROGRAM)/poller
EXECCLIENT = $(PROGRAM)/pollSwayer

all: $(EXECSERVER) $(EXECCLIENT)

$(EXECSERVER): $(OBJSSERVER)
	$(CC) $(OBJSSERVER) -o $(EXECSERVER)

$(EXECCLIENT): $(OBJSCLIENT)
	$(CC) $(OBJSCLIENT) -o $(EXECCLIENT)

clean:
	rm -f $(OBJSSERVER) $(EXECSERVER) $(OBJSCLIENT) $(EXECCLIENT)

server: $(EXECSERVER)
	$(EXECSERVER) $(ARGSSERVER)

valserver: $(EXECSERVER)
	valgrind --leak-check=full --track-origins=yes $(EXECSERVER) $(ARGSSERVER)

client: $(EXECCLIENT)
	$(EXECCLIENT) $(ARGSCLIENT)

valclient: $(EXECCLIENT)
	valgrind --leak-check=full --track-origins=yes $(EXECCLIENT) $(ARGSCLIENT)
