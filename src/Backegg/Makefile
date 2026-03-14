CC=cc
GCC=g++

.PHONY: all clean

all: libbackend.a

libbackend.a: back.o
	ar rcs libbackend.a back.o

back.o: gen_asm.c
	$(GCC) -g -I /usr/include/llvm-c-17/ `llvm-config-17 --cxxflags` -c gen_asm.c -o back.o

clean:
	rm -f libbackend.a *.o *.out
