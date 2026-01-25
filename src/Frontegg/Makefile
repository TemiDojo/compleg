CC=cc
GCC=g++
LEX=lex
YACC=yacc
FLAGS=-o
.PHONY: all clean

all: frontend

frontend: lex.yy.c y.tab.c ast.o
	$(GCC) $^ $(FLAGS) $@ 

lex.yy.c: frontend.l
	$(LEX) $^

y.tab.c: frontend.y
	$(YACC) -Wconflicts-sr -Wcounterexamples -d $^

ast.o: ../ast/ast.c
	$(GCC) -c $^

clean:
	rm -f frontend *.yy.c y.tab.* ast.o


