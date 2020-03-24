# Brandon Lewien
# Lab 0

CC = g++
CFLAGS = -c -g -Wall -Wextra
LFLAGS = -std=c++11 -g -pthread

mysort: mysort.cpp 
	$(CC) $(LFLAGS) $^ -o $@

counter: counter.cpp 
	$(CC) $(LFLAGS) $^ -o $@

all: counter mysort 

clean:
	rm -f mysort
	rm -f counter
	rm -f *.o
