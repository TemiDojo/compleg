GCC=g++

.PHONY: all clean

all: libmiddle.a	

libmiddle.a: opt.o
	ar rcs libmiddle.a opt.o
opt.o: opt.c
	$(GCC) -g -I /usr/include/llvm-c-17/ `llvm-config-17 --cxxflags` -c opt.c -o opt.o
	
clean:
	rm -f libmiddle.a *.o
