# Brandon Lewien
# Project - Queues/Stacks

EXE = containers
CC = g++
LFLAGS = -std=c++11 -g -pthread

all: $(EXE)

containers.o: containers.cpp
	$(CC) $(LFLAGS) -c -o containers.o containers.cpp

sgl.o: sgl.cpp sgl.h
	$(CC) $(LFLAGS) -c -o sgl.o sgl.cpp

treiber.o: treiberstack.cpp treiberstack.h
	$(CC) $(LFLAGS) -c -o treiber.o treiberstack.cpp

msqueue.o: msqueue.cpp msqueue.h
	$(CC) $(LFLAGS) -c -o msqueue.o msqueue.cpp

basketqueue.o: basketqueue.cpp basketqueue.h
	$(CC) $(LFLAGS) -c -o basketqueue.o basketqueue.cpp

eliminationstack.o: eliminationstack.cpp eliminationstack.h
	$(CC) $(LFLAGS) -c -o eliminationstack.o eliminationstack.cpp

containers: containers.o sgl.o treiber.o msqueue.o basketqueue.o eliminationstack.o
	$(CC) -o $(EXE) $(LFLAGS) containers.o sgl.o treiber.o msqueue.o basketqueue.o eliminationstack.o


clean:
	rm -f $(EXE)
	rm -f *.o
