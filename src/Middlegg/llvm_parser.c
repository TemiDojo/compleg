#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#include <unordered_map>
#include <vector>
#include <cstddef>
#include <string>

#define prt(x) if(x) { printf("%s\n", x); }

/* This function reads the given llvm file and loads the LLVM IR into
 * data-structures that we can work on for optimization phase.
 */


LLVMModuleRef createLLVMModel(char * filename) {
	char *err = 0;

	LLVMMemoryBufferRef ll_f = 0;
	LLVMModuleRef m = 0;

	LLVMCreateMemoryBufferWithContentsOfFile(filename, &ll_f, &err);

	if (err != NULL) {
		prt(err);
		return NULL;
	}

	LLVMParseIRInContext(LLVMGetGlobalContext(), ll_f, &m, &err);

	if (err != NULL) {
		prt(err);
	}

	return m;
}

void printMap(std::unordered_map<std::string, LLVMValueRef> *m) {
	std::unordered_map<std::string, LLVMValueRef> dum = *m;
	std::unordered_map<std::string, LLVMValueRef>::iterator it = dum.begin();
	while(it != dum.end()) {
		printf("PRINTING: %s\n", it->first.c_str());		
		it++;
	}
}
void printMap2(std::unordered_map<std::string, std::vector<LLVMValueRef>> *m) {
	std::unordered_map<std::string, std::vector<LLVMValueRef>> dum = *m;
	std::unordered_map<std::string, std::vector<LLVMValueRef>>::iterator it = dum.begin();
	while(it != dum.end()) {
		printf("PRINTING: %s:  ->", it->first.c_str());		
		std::vector<LLVMValueRef>::iterator it2 = it->second.begin();
		printf("[");
		while(it2 != it->second.end()) {
			printf("%s, ", LLVMPrintValueToString(*it2));
			it2++;
		}
		puts("]");
		it++;
	}
}

bool common_subexpr(LLVMBasicBlockRef bb) {
	bool ret = false;
	std::unordered_map<std::string, LLVMValueRef> m; 
	for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction; instruction = LLVMGetNextInstruction(instruction)) {

		// before we add we find
		std::string key = LLVMPrintValueToString(instruction);

		if (LLVMGetInstructionOpcode(instruction) == LLVMAlloca) {
			m[key] = instruction;
		} else {

			size_t pos = key.find('=');
			if (pos != std::string::npos) {
				key = key.substr(pos + 1);
			}

			if (m.count(key)) {
				//printf("key exists '%s' in map\n", key.c_str());
				LLVMReplaceAllUsesWith(instruction, m.at(key));
				ret = true;
				puts("subexpr still made");
			} else {
				//printf("adding string '%s' in map\n", key.c_str());
				if (LLVMGetInstructionOpcode(instruction) == LLVMStore) {
					std::unordered_map<std::string, LLVMValueRef>::iterator it = m.begin();	
					while(it != m.end()) {
						if (LLVMGetInstructionOpcode(it->second) == LLVMLoad) {
							int size = LLVMGetNumOperands(it->second);
							LLVMValueRef cmp = LLVMGetOperand(it->second, size-1);
							if (cmp == LLVMGetOperand(instruction, 1)) {
								m.erase(it->first);
								break;
							}
						}
						it++;
					}
				}
				m[key] = instruction;
			}
		}

	}
	//printMap(&m);
	printf("ret val is: %d\n", ret);
	return ret;

}

bool isValid(LLVMOpcode opcode) {
	switch(opcode) {

		case LLVMRet:
		case LLVMBr:
		case LLVMIndirectBr:
		case LLVMInvoke:
		case LLVMStore:
		case LLVMCall:
			return false;
		default:
			break;
	}

	return true;
}

bool deadcodeElim(LLVMBasicBlockRef bb) {
	bool ret = false;
	std::unordered_map<std::string, std::vector<LLVMValueRef>> m;
	for(LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction; instruction = LLVMGetNextInstruction(instruction)){
		std::string key = LLVMPrintValueToString(instruction);
		LLVMOpcode opcode = LLVMGetInstructionOpcode(instruction);
		if (!m.count(key)) {
			std::vector<LLVMValueRef> neighbors;
			m[key] = neighbors;
			int op_count = LLVMGetNumOperands(instruction);
			if (op_count > 0) {
				for(int i = 0; i < op_count; i++) {
					LLVMValueRef val = LLVMGetOperand(instruction, i); 
					std::string token = LLVMPrintValueToString(val);

					if (m.count(token)) {
						m[token].push_back(instruction);
					}
				}
			}

		}


	}
	//printMap2(&m);

	for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction; instruction = LLVMGetNextInstruction(instruction)) {
		std::string key = LLVMPrintValueToString(instruction);
		LLVMOpcode opcode = LLVMGetInstructionOpcode(instruction);
		if (isValid(opcode) && m[key].size() == 0) {
			printf("removing: %s\n", LLVMPrintValueToString(instruction));
			LLVMInstructionEraseFromParent(instruction);
			ret = true;
			puts("change still made");
		}

	}
	printf("ret val2 is: %d\n", ret);
	return ret;

}

bool isvalidOperand(LLVMValueRef instruction, int *const) {
	/*
	int dummy = *const;
	for (int i = 0; i < size; i++) {
		LLVMValueRef val = LLVMGetOperand(instruction, i);
		if (LLVMIsConstant(val)) {
			dummy
			cont
		}

	}
	*/

}

bool constantFold(LLVMBasicBlockRef bb) {

	for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction; instruction = LLVMGetNextInstruction(instruction)) {
		LLVMOpcode Opcode = LLVMGetInstructionOpcode(instruction);
		switch(Opcode) {
			case LLVMAdd:
				//int size = LLVMGetNumOperands(instruction);
				break;
			case LLVMSub:
				break;
			case LLVMMul:
				break;
			default:
				break;


		}
		

	}
	return false;

}

void walkBasicblocks(LLVMValueRef function) {
	bool change = false;
	for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
		printf("In basic block\n");
		while(true) {
			change |= common_subexpr(basicBlock);
			change |= deadcodeElim(basicBlock);
			if (!change) {
				break;
			}
			change = false;
		}

	}
}

void walkFunctions(LLVMModuleRef module) {
	for (LLVMValueRef function = LLVMGetFirstFunction(module); function; function = LLVMGetNextFunction(function)) {
		const char* funcName = LLVMGetValueName(function);
		printf("Function Name: %s\n", funcName);

		walkBasicblocks(function);
	}
}


int main(int argc, char** argv) {

	LLVMModuleRef m;

	if (argc == 2) {
		m = createLLVMModel(argv[1]);
	}
	else {
		m = NULL;
		return 1;
	}
	if (m != NULL) {
		walkFunctions(m);

	} else {
		fprintf(stderr, "m is NULL\n");
	}
	//LLVMPrintModuleToFile(m, argv[1], NULL);
	char *res = LLVMPrintModuleToString(m);
	printf("%s\n", res);
	LLVMDisposeMessage(res);

	return 0;
}
