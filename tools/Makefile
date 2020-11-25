CC      = gcc
CFLAGS  = -Wall -g -std=c99 -Werror 

.SUFFIXES: .c .o

.PHONY: all clean

all: asm emu

asm: adts.o assemble.o
	$(CC) adts.o assemble.o -o asm

emu: emulate.o 
	$(CC) emulate.o -o emu

emulate.o: emulate.c
	$(CC) $(CFLAGS) emulate.c -c -o emulate.o

assemble.o: assemble.c
	$(CC) $(CFLAGS) assemble.c -c -o assemble.o

adts.o: adts.h adts.c
	$(CC) $(CFLAGS) adts.c -c -o adts.o

clean:
	rm -f $(wildcard *.o)
	rm -f asm
	rm -f emu
