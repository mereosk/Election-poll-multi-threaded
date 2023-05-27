# paths
INCLUDE = ./include
MODULES = ./modules
PROGRAM = ./program
# compiler
CC = gcc

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CFLAGS = -Wall  -g -I$(INCLUDE)
LDFLAGS = 
ARGS = 3 2 1 poll-log.txt poll-stats.txt

# Αρχεία .o
OBJS = $(PROGRAM)/poller.o $(MODULES)/ADTList.o $(MODULES)/ADTVector.o $(MODULES)/ADTMap.o $(MODULES)/helpingFuncs.o

# Το εκτελέσιμο πρόγραμμα
EXEC = $(PROGRAM)/poller

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC)

clean:
	rm -f $(OBJS) $(EXEC)

run: $(EXEC)
	$(EXEC) $(ARGS)

valgrind: $(EXEC)
	valgrind --leak-check=full --track-origins=yes $(EXEC) $(ARGS)
