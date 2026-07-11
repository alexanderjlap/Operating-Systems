CFLAGS = -g -Wall
LIB = -lpthread
CC = gcc

all: proj2 doit

doit: doit.cpp
	g++ $(CFLAGS) doit.cpp -o doit

proj2: project2.o 
	$(CC) $(CFLAGS) project2.o -o proj2 $(LIB)

project2.o: project2.c 
	$(CC) $(CFLAGS) -c project2.c -o project2.o

clean:
	rm -f proj2 project2.o doit