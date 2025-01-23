# Election-poll-multi-threaded

This document provides an overview of my project, including its structure, key components, execution steps, and core functionality. Below is a summarized breakdown of the content:  

### Summary of Each Section  
1. **Directories and File Structure:** Overview of the program's folders and their contents.  
2. **Makefile and Execution:** Explanation of the Makefile and how the program can be run in different ways.  
3. **Core Functionality:** Description of the two main `map` structures and the role of the master thread in `poller`.  
4. **Buffer Queue Implementation:** Details on the queue, its use as a buffer, and avoiding busy waiting in the master thread.  
5. **Worker Threads:** Explanation of worker thread functionality, avoidance of busy waiting, and the use of mutexes for thread safety.  
6-8. **Graceful Termination (SIGINT):** How `poller` and worker threads properly handle termination when SIGINT is received.  
9. **PollSwayer Program:** Implementation details, including thread handling and the vector used to store `pthreadId` values.  
10. **Input Constraints:** Note on input limitations (names and party names up to 64 characters) for proper program behavior.  

### File Structure and Compilation  

The project consists of the following directories:  

- **`include/`**: Contains header files defining the function prototypes used throughout the project.  
- **`modules/`**: Contains implementation files such as `helpingFuncs.c`, which includes utility functions like socket handling and argument validation. Additionally, it includes data structure implementations: `ADTList.c`, `ADTMap.c`, `ADTVector.c`, and `ADTQueue.c`. These are inspired by existing designs but have been significantly modified for project needs.  
- **`program/`**: Contains the main programs, `poller.c` and `pollSwayer.c`.  
- **`scripts/`**: Contains scripts for input generation and result processing:  
  - `createInput.sh` (modified to camelCase).  
  - `processLogFile.sh`.  
  - `tallyVotes.sh`.  
  - `myCreateInput.sh` (used for debugging and generating Greek names).  
- **`misc/`**: Contains text files that hold input data, intermediate results, and final outputs.  

The project includes a **Makefile** for managing compilation and execution:  
- `make server` and `make client` compile and run the server and client programs, respectively.  
- `make valserver` and `make valclient` run the programs with `valgrind` to check for memory leaks or errors.  
- `make clean` removes object and executable files.  

**Execution Examples**:  
1. Standard execution with `createInput.sh` and processing results with scripts.  
2. Test case execution with predefined inputs.  
3. Execution with `myCreateInput.sh`, which generates input with realistic Greek names.  

### Poller Program  

The `poller` program implements a multithreaded TCP server that handles multiple clients simultaneously.  

**Core Features:**  
- **Two Maps:**  
  - `statsMap`: Tracks party names and their corresponding vote counts.  
  - `checkMap`: Ensures voters don't vote multiple times by storing their names.  
- **Buffer Queue:**  
  - A queue is used as a buffer for sockets. The master thread adds sockets to the queue, while worker threads process them.  
  - Avoids busy waiting with `pthread_cond_wait` and condition variables (`cond_nonfull`, `cond_nonempty`).  

**Worker Threads:**  
Each worker thread performs the following:  
1. Waits for a socket to become available in the buffer.  
2. Processes the client request, updating `statsMap` and `checkMap` with appropriate mutex protection (`checkMapMtx`, `statsMapMtx`).  
3. Writes results to `poll-log.txt`, ensuring thread-safe access with `writeMtx`.  

**Graceful Termination:**  
When SIGINT is received:  
- The master thread sets a flag (`sigintFlag`) and broadcasts a signal to all waiting threads.  
- Worker threads terminate upon detecting the flag, ensuring no resources are leaked.  
- The master thread exits the main loop, waits for all worker threads to complete, and writes final results to `poll-stats.txt`.  

### PollSwayer Program  

The `pollSwayer` program acts as a multithreaded client that communicates with the `poller` server.  

**Features:**  
- Uses a dynamic vector to manage thread creation and joining.  
- Each thread processes a line from the input file, extracting the voter name and party before initiating a connection to the server.  
- Ensures clean communication with the server and proper resource handling.  

### Input Constraints  

The program supports names and party names up to 64 characters. Input longer than this may result in undefined behavior. This limitation will be addressed in a future version.  
