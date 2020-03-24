# Brandon Lewien
# Lab 0

CC = gcc
CFLAGS = -c -g -Wall -Wextra
LFLAGS = -g -Wall -Wextra

mysort: mysort.o
	$(CC) $(LFLAGS) $^ -o $@

clean:
	rm -f mysort
	rm -f *.o
	rm -f *.txt