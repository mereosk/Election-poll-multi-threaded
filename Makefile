# paths
INCLUDE = ./include
MODULES = ./modules
PROGRAM = ./program
MISC = ./misc
# compiler
CC = gcc

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CFLAGS = -w -I$(INCLUDE)
LDFLAGS = 
ARGSSERVER = 5635 5 10 $(MISC)/poll-log.txt $(MISC)/poll-stats.txt
ARGSCLIENT = mereosk-Latitude-E7440 5635 $(MISC)/inputFile.txt

# Αρχεία .o
OBJSSERVER = $(PROGRAM)/poller.o $(MODULES)/ADTList.o $(MODULES)/ADTVector.o $(MODULES)/ADTMap.o $(MODULES)/ADTQueue.o $(MODULES)/helpingFuncs.o
OBJSCLIENT = $(PROGRAM)/pollSwayer.o $(MODULES)/helpingFuncs.o $(MODULES)/ADTVector.o

# Τα εκτελέσιμα πρόγραμματα
EXECSERVER = $(PROGRAM)/poller
EXECCLIENT = $(PROGRAM)/pollSwayer

all: $(EXECSERVER) $(EXECCLIENT)

$(EXECSERVER): $(OBJSSERVER)
	$(CC) $(OBJSSERVER) -o $(EXECSERVER) -lpthread 

$(EXECCLIENT): $(OBJSCLIENT)
	$(CC) $(OBJSCLIENT) -o $(EXECCLIENT) -lpthread

clean:
	rm -f $(OBJSSERVER) $(EXECSERVER) $(OBJSCLIENT) $(EXECCLIENT)

server: $(EXECSERVER)
	$(EXECSERVER) $(ARGSSERVER)

valserver: $(EXECSERVER)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(EXECSERVER) $(ARGSSERVER)

client: $(EXECCLIENT)
	$(EXECCLIENT) $(ARGSCLIENT)

valclient: $(EXECCLIENT)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(EXECCLIENT) $(ARGSCLIENT)
