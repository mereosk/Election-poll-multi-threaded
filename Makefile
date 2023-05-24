# paths
INCLUDE = ./include
MODULES = ./modules
PROGRAM = ./program
# compiler
CC = gcc

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CFLAGS = -Wall  -g -I$(INCLUDE)
LDFLAGS = 

# Αρχεία .o
OBJS = $(PROGRAM)/main.o $(MODULES)/ADTList.o $(MODULES)/ADTVector.o $(MODULES)/ADTMap.o

# Το εκτελέσιμο πρόγραμμα
EXEC = $(PROGRAM)/main

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC)

clean:
	rm -f $(OBJS) $(EXEC)

run: $(EXEC)
	$(EXEC)

valgrind: $(EXEC)
	valgrind --leak-check=full --track-origins=yes $(EXEC)
