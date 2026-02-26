CC=cc
GCC=g++
LEX=lex
YACC=yacc
FLAGS=-o
.PHONY: all clean

all: libfrontend.a

lex.yy.o: lex.yy.c y.tab.h
	$(GCC) -c lex.yy.c -o lex.yy.o

y.tab.o: y.tab.c
	$(GCC) -c y.tab.c -o y.tab.o
	
semantic.o: semantic.c
	$(GCC) -c semantic.c -o semantic.o

libfrontend.a: lex.yy.o y.tab.o semantic.o
	ar rcs libfrontend.a lex.yy.o y.tab.o semantic.o

lex.yy.c: frontend.l y.tab.h
	$(LEX) frontend.l

y.tab.c y.tab.h: frontend.y
	$(YACC) -Wconflicts-sr -Wcounterexamples -d frontend.y

clean:
	rm -f libfrontend.a *.yy.c y.tab.* *.o


