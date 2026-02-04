LLVMCODE = llvm_parser
TEST = test_common_subexpr

.PHONY: all clean

all:	$(LLVMCODE) llvm_file

$(LLVMCODE): $(LLVMCODE).c
	g++ -g -I /usr/include/llvm-c-17/ -c $(LLVMCODE).c
	g++ $(LLVMCODE).o `llvm-config-17 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-17/ -o $@
	
llvm_file: $(TEST).c
	clang -S -emit-llvm $(TEST).c -o $(TEST).ll

clean:
	rm -rf $(TEST).ll
	rm -rf $(LLVMCODE)
	rm -rf *.o
