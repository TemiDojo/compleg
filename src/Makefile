CC=cc
GCC=g++

# Directory Variables
FRONT_DIR = ./Frontegg
MID_DIR = ./Middlegg
BAC_DIR = ./Backegg
AST_DIR = ./ast

FRONT_LIB = $(FRONT_DIR)/libfrontend.a
MID_LIB = $(MID_DIR)/libmiddle.a
BAC_LIB = $(BAC_DIR)/libbackend.a
AST_OBJ = $(AST_DIR)/ast.o

INC = -I$(FRONT_DIR) -I$(AST_DIR)

all: compiler

# link everything together
compiler: entry.c $(AST_OBJ) $(FRONT_LIB) $(MID_LIB) $(BAC_LIB)
	$(GCC) entry.c $(AST_OBJ) $(FRONT_LIB) $(MID_LIB) $(BAC_LIB) $(INC) `llvm-config-17 --cxxflags --ldflags --libs core` -o compiler

# build the parser library
$(FRONT_LIB):
	$(MAKE) -C $(FRONT_DIR)

# build the optimizatio nlibrary
$(MID_LIB):
	$(MAKE) -C $(MID_DIR)

# build the backend library
$(BAC_LIB):
	$(MAKE) -C $(BAC_DIR)

# build the AST object file
$(AST_OBJ):
	$(MAKE) -C $(AST_DIR)

clean:
	rm -f compiler *.ll
	$(MAKE) -C $(FRONT_DIR) clean
	$(MAKE) -C $(AST_DIR) clean
	$(MAKE) -C $(MID_DIR) clean
	$(MAKE) -C $(BAC_DIR) clean
