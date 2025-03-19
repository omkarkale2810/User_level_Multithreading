# Makefile for user-level threading library

CC = gcc
CFLAGS = -Wall -g -pthread

all: test_threads

# Compile the threading library
threading.o: threading.c threading.h
	$(CC) $(CFLAGS) -c threading.c -o threading.o

# Compile the test program
test_threads.o: test_threads.c threading.h
	$(CC) $(CFLAGS) -c test_threads.c -o test_threads.o

# Link everything together
test_threads: test_threads.o threading.o
	$(CC) $(CFLAGS) test_threads.o threading.o -o test_threads

clean:
	rm -f *.o test_threads
