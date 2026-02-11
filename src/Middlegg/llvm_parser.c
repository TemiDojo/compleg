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
#include <set>

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
				//puts("subexpr still made");
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
	//printf("ret val is: %d\n", ret);
	return ret;

}

bool isValid(LLVMOpcode opcode) {
	switch(opcode) {

		case LLVMRet:
		case LLVMBr:
		case LLVMIndirectBr:
		case LLVMInvoke:
		case LLVMStore:
		case LLVMAlloca:
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
			LLVMInstructionEraseFromParent(instruction);
			ret = true;
		}

	}
	//printf("ret val2 is: %d\n", ret);
	return ret;

}


bool constantFold(LLVMBasicBlockRef bb) {
	bool ret = false;
	long long dummyRHS = 0;  
	long long dummyLHS = 0;
	for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction; instruction = LLVMGetNextInstruction(instruction)) {
		LLVMOpcode Opcode = LLVMGetInstructionOpcode(instruction);
		switch(Opcode) {
			case LLVMAdd: {
				LLVMValueRef valLHS = LLVMGetOperand(instruction, 0);
				LLVMValueRef valRHS = LLVMGetOperand(instruction, 1);
				if (LLVMIsConstant(valLHS) && LLVMIsConstant(valRHS)) {
					dummyLHS = LLVMConstIntGetSExtValue(valLHS);
					dummyRHS = LLVMConstIntGetSExtValue(valRHS);
					valLHS = LLVMConstInt(LLVMInt32Type(), dummyLHS, 1);
					valRHS = LLVMConstInt(LLVMInt32Type(), dummyRHS, 1);
					LLVMValueRef new_ins = LLVMConstAdd(valLHS, valRHS);
					LLVMReplaceAllUsesWith(instruction, new_ins);
					ret = true;
				} else { 
					ret = false;
				}

				break;
		      }
			case LLVMSub:{
				
					LLVMValueRef valLHS = LLVMGetOperand(instruction, 0);
					LLVMValueRef valRHS = LLVMGetOperand(instruction, 1);
					if (LLVMIsConstant(valLHS) && LLVMIsConstant(valRHS)) {
						dummyLHS = LLVMConstIntGetSExtValue(valLHS);
						dummyRHS = LLVMConstIntGetSExtValue(valRHS);
						valLHS = LLVMConstInt(LLVMInt32Type(), dummyLHS, 1);
						valRHS = LLVMConstInt(LLVMInt32Type(), dummyRHS, 1);
						LLVMValueRef new_ins = LLVMConstSub(valLHS, valRHS);
						LLVMReplaceAllUsesWith(instruction, new_ins);
						ret = true;
					} else {
						ret = false;
					}
					break;
				     }
			case LLVMMul:{
					LLVMValueRef valLHS = LLVMGetOperand(instruction, 0);
					LLVMValueRef valRHS = LLVMGetOperand(instruction, 1);
					if (LLVMIsConstant(valLHS) && LLVMIsConstant(valRHS)) {
						dummyLHS = LLVMConstIntGetSExtValue(valLHS);
						dummyRHS = LLVMConstIntGetSExtValue(valRHS);
						valLHS = LLVMConstInt(LLVMInt32Type(), dummyLHS, 1);
						valRHS = LLVMConstInt(LLVMInt32Type(), dummyRHS, 1);
						LLVMValueRef new_ins = LLVMConstMul(valLHS, valRHS);
						LLVMReplaceAllUsesWith(instruction, new_ins);
						ret = true;
					} else {
						ret = false;
					}
					     
					break;
				     }
			default:
				     ret = false;
				break;


		}
		

	}

	return ret;

}
void printGenTable(std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *m) {

	std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> dum = *m;
	for (auto i : dum) {

		LLVMDumpValue(LLVMBasicBlockAsValue(i.first));
		//printf("PRINTING: %s:  ->", LLVMGetBasicBlockName(i.first));		
		printf("[");
		for (auto j : i.second) {
			printf("%s, ", LLVMPrintValueToString(j));
		}
		puts("]");
	}
}

void gen(LLVMBasicBlockRef bb, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *genTable, std::set<LLVMValueRef> *iSet, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>> *predMap, std::set<LLVMValueRef> *gSet) {

	std::set<LLVMValueRef> s;
	std::set<LLVMBasicBlockRef> ss;
	std::unordered_map<LLVMValueRef, LLVMValueRef> mcheck;
	(*genTable)[bb] = s;

	for(LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction; instruction = LLVMGetNextInstruction(instruction)) {
		(*gSet).insert(instruction);
		LLVMOpcode op = LLVMGetInstructionOpcode(instruction);
		if (op == LLVMStore) {
			(*iSet).insert(instruction);
			LLVMValueRef ptr = LLVMGetOperand(instruction, 1);
			if (mcheck.count(ptr)) {
				(*genTable)[bb].erase(mcheck[ptr]);
				(*genTable)[bb].insert(instruction);
				mcheck[ptr] = instruction;
			} else {
				mcheck[ptr] = instruction;
				(*genTable)[bb].insert(instruction);
			}
		}
		if (LLVMIsATerminatorInst(instruction)) {
			unsigned int suc_num = LLVMGetNumSuccessors(instruction);
			for (unsigned int i = 0; i < suc_num; i++) {
				LLVMBasicBlockRef bnew = LLVMGetSuccessor(instruction, i);
				if ((*predMap).count(bnew)) {
					(*predMap)[bnew].insert(bb);
				} else {
					std::set<LLVMBasicBlockRef> sss;
					(*predMap)[bnew] = sss;
					(*predMap)[bnew].insert(bb);
				}

			}

		}
	}
	//printGenTable(&dummy);

}

void kill(std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *genTable, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *killTable, std::set<LLVMValueRef> *iSet) {

	for (auto i : (*genTable)) {
		std::set<LLVMValueRef> s;
		(*killTable)[i.first] = s;
		for (LLVMValueRef instruction = LLVMGetFirstInstruction(i.first); instruction; instruction = LLVMGetNextInstruction(instruction)) {

			LLVMOpcode opcode = LLVMGetInstructionOpcode(instruction);
			if (opcode == LLVMStore) {
				LLVMValueRef op = LLVMGetOperand(instruction, 0);
				LLVMValueRef op2 = LLVMGetOperand(instruction, 1);
				for (auto k : (*iSet)) {
					LLVMValueRef optocmp = LLVMGetOperand(k, 0);
					LLVMValueRef optocmp2 = LLVMGetOperand(k, 1);
					if (op == optocmp && op2 == optocmp2) {
						continue;
					} else if (optocmp2 == op2) {
						(*killTable)[i.first].insert(k);
					}
				}
			}
		}
	}
}

bool constantProp(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *in) {
	
	bool ret = false;
	//printGenTable(&(*in));
	std::set<LLVMValueRef> R= (*in)[basicBlock];
	std::set<LLVMValueRef> tbd;
	for(LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock); instruction; instruction = LLVMGetNextInstruction(instruction)) {
		LLVMOpcode opcode = LLVMGetInstructionOpcode(instruction);
		if (opcode == LLVMStore) {
			if (!R.count(instruction)) {
				R.insert(instruction);
			} else {
				LLVMValueRef op = LLVMGetOperand(instruction, 1);
				std::set<LLVMValueRef> nR(R);
				for (auto i : nR) {
					LLVMValueRef op2 = LLVMGetOperand(i, 1);
					if (op == op2) {
						R.erase(i);
					}
				}
				R.insert(instruction);
			}


		} else if (opcode == LLVMLoad) {
			LLVMValueRef op = LLVMGetOperand(instruction, 0);
			std::set<long long> cR;
			bool flag = true;
			long long val;
			for (auto j : R) {
				LLVMValueRef op2 = LLVMGetOperand(j, 1);	
				LLVMValueRef op1 = LLVMGetOperand(j, 0);
				if (op2 == op && LLVMIsAConstant(op1)) {
					val = LLVMConstIntGetSExtValue(op1);
					cR.insert(val);
				} else if (op2 == op && !LLVMIsAConstant(op1)) {
					cR.clear();
					break;
				}
			}

			if (cR.size() == 1) {
				LLVMValueRef rep = LLVMConstInt(LLVMInt32Type(), val, 1);
				LLVMReplaceAllUsesWith(instruction, rep);
				tbd.insert(instruction);
				ret = true;
				flag = true;

			}
		}
	}

	for (auto k : tbd) {
		//printf("LOAD 2 DELETE: %s\n", LLVMPrintValueToString(k));
		LLVMInstructionEraseFromParent(k);

	}

	return ret;


}

bool cmpB(LLVMValueRef function, std::set<LLVMValueRef> *gSet) {
	std::set<LLVMValueRef> gCmp;
	for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {

		for (LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock); instruction; instruction = LLVMGetNextInstruction(instruction)){
			gCmp.insert(instruction);	
		}

	}
	if ((*gSet)!=gCmp) {
		puts("in hre");
		return true;
	}
	return false;
}

bool globalOpt(LLVMValueRef function) {

	std::unordered_map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>> predMap;
	std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> genTable;
	std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> killTable;
	std::set<LLVMValueRef> iSet;
	std::set<LLVMValueRef> gSet;

	std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> in;
	std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> out;

	for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
		// gen set
		gen(basicBlock, &genTable, &iSet, &predMap, &gSet);
		std::set<LLVMValueRef> s;
		in[basicBlock] = s;
	}

	kill(&genTable, &killTable, &iSet);
	//printGenTable(&genTable);
	//printGenTable(&killTable);
	// gen map of predecessors
	for (auto i : genTable) {
		std::set<LLVMValueRef> newset(i.second);
		out[i.first] = newset;
	}
	bool change = true;
	while (change) {
		
		change = false;

		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
			for (auto i : predMap[basicBlock]) {
				in[basicBlock].insert(out[i].begin(), out[i].end());
			}
			std::set<LLVMValueRef> oldout(out[basicBlock]);
			
			std::set<LLVMValueRef>ss(in[basicBlock]);
			std::set<LLVMValueRef>sss(in[basicBlock]);
			for (auto j : ss) {
				if (killTable[basicBlock].count(j)) {
					sss.erase(j);
				}
			}
			out[basicBlock].clear();
			out[basicBlock].insert(genTable[basicBlock].begin(), genTable[basicBlock].end());
			out[basicBlock].insert(sss.begin(), sss.end());

			if (oldout != out[basicBlock]) {
				change = true;
			}
		}
	}
	bool opt_change = false;
	for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
		while(true) {
			opt_change |= constantFold(basicBlock);
			opt_change |= constantProp(basicBlock, &in);
			

			// constant fold

			puts("word");
			if (!opt_change) {
				break;
			}
			opt_change = false;
		}

	}


	return cmpB(function, &gSet);


}

void walkBasicblocks(LLVMValueRef function) {

	bool loc_change = false;
	bool glob_change = false;
	while(1) {
		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
			while(true) {
				loc_change |= common_subexpr(basicBlock);
				loc_change |= deadcodeElim(basicBlock);
				loc_change |= constantFold(basicBlock);
				if (!loc_change) {
					break;
				}
				loc_change = false;
			}

		}
		
		glob_change |= globalOpt(function);
		puts("checking");
		
		if (!glob_change) {
			break;
		}
		glob_change = false;
	}
	
}

void walkFunctions(LLVMModuleRef module) {
	for (LLVMValueRef function = LLVMGetFirstFunction(module); function; function = LLVMGetNextFunction(function)) {
		const char* funcName = LLVMGetValueName(function);
		//printf("Function Name: %s\n", funcName);

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
