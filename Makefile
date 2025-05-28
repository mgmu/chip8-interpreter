all: chip

chip: main.c interpreter.o
	gcc main.c interpreter.o -o chip

interpreter.o: interpreter.c interpreter.h
	gcc interpreter.c -o interpreter.o

clean:
	rm -rf *~ *.o chip
