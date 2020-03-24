# Brandon Lewien
# Lab 0

CC = g++
CFLAGS = -c -g -Wall -Wextra
LFLAGS = -fopenmp

mysort: mysort.cpp 
	$(CC) $(LFLAGS) $^ -o $@

clean:
	rm -f mysort
	rm -f counter
	rm -f *.o
