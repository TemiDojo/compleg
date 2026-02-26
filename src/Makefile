CC=cc
GCC=g++

# Directory Variables
FRONT_DIR = ./Frontegg
MID_DIR = ./Middlegg
AST_DIR = ./ast

FRONT_LIB = $(FRONT_DIR)/libfrontend.a
MID_LIB = $(MID_DIR)/libmiddle.a
AST_OBJ = $(AST_DIR)/ast.o

INC = -I$(FRONT_DIR) -I$(AST_DIR)

all: compiler

# link everything together
compiler: entry.c $(AST_OBJ) $(FRONT_LIB)
	$(GCC) entry.c $(AST_OBJ) $(FRONT_LIB) $(INC) -o compiler

# build the parser library
$(FRONT_LIB):
	$(MAKE) -C $(FRONT_DIR)

# build the AST object file
$(AST_OBJ):
	$(MAKE) -C $(AST_DIR)

clean:
	rm -f compiler
	$(MAKE) -C $(FRONT_DIR) clean
	$(MAKE) -C $(AST_DIR) clean
