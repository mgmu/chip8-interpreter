all: chip

chip: main.c
	gcc main.c -o chip

clean:
	rm -rf *~ chip
