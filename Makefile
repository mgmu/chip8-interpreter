all: chip

chip: main.o interpreter.o
	gcc -o chip8 main.o interpreter.o

main.o: main.c
	gcc -c main.c

interpreter.o: interpreter.c interpreter.h
	gcc -c interpreter.c

clean:
	rm -rf *~ *.o chip8
