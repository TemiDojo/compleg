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
	std::set<LLVMValueRef> R = (*in)[basicBlock]; 
	std::set<LLVMValueRef> tbd;
	
	for(LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock); instruction; instruction = LLVMGetNextInstruction(instruction)) {
		LLVMOpcode opcode = LLVMGetInstructionOpcode(instruction);
		
		if (opcode == LLVMStore) {
			R.insert(instruction);
			
			LLVMValueRef store_addr = LLVMGetOperand(instruction, 1); 
			std::set<LLVMValueRef> to_remove;
			
			for (auto r_inst : R) {
				if (r_inst == instruction) continue; 
				
				if (LLVMGetInstructionOpcode(r_inst) == LLVMStore) {
					LLVMValueRef r_addr = LLVMGetOperand(r_inst, 1);
					if (store_addr == r_addr) {
						to_remove.insert(r_inst);
					}
				}
			}
			
			for (auto rem : to_remove) {
				R.erase(rem);
			}
			
		} else if (opcode == LLVMLoad) {
			LLVMValueRef load_addr = LLVMGetOperand(instruction, 0); 
			
			std::set<LLVMValueRef> reaching_stores;
			for (auto r_inst : R) {
				if (LLVMGetInstructionOpcode(r_inst) == LLVMStore) {
					LLVMValueRef store_addr = LLVMGetOperand(r_inst, 1);
					if (store_addr == load_addr) {
						reaching_stores.insert(r_inst);
					}
				}
			}
			
			if (!reaching_stores.empty()) {
				bool all_constant = true;
				long long constant_value = 0;
				bool first = true;
				
				for (auto store_inst : reaching_stores) {
					LLVMValueRef stored_val = LLVMGetOperand(store_inst, 0);
					
					if (!LLVMIsConstant(stored_val)) {
						all_constant = false;
						break;
					}
					
					long long val = LLVMConstIntGetSExtValue(stored_val);
					
					if (first) {
						constant_value = val;
						first = false;
					} else if (val != constant_value) {
						all_constant = false;
						break;
					}
				}
				
				if (all_constant) {
					LLVMValueRef replacement = LLVMConstInt(LLVMInt32Type(), constant_value, 1);
					LLVMReplaceAllUsesWith(instruction, replacement);
					tbd.insert(instruction);
					ret = true;
				}
			}
		}
	}
	
	for (auto inst : tbd) {
		LLVMInstructionEraseFromParent(inst);
	}
	
	return ret;
}



bool globalOpt(LLVMValueRef function) {
	bool ret = false;
	int iteration = 0;
	while(true) {
		iteration++;
		//printf("Global opt iteration %d\n", iteration);
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
				in[basicBlock].clear();
				for (auto i : predMap[basicBlock]) {
					in[basicBlock].insert(out[i].begin(), out[i].end());
				}
				std::set<LLVMValueRef> oldout(out[basicBlock]);
				
				std::set<LLVMValueRef>temp(in[basicBlock]);
				for (auto j : killTable[basicBlock]) {
					temp.erase(j);
				}
				out[basicBlock].clear();
				out[basicBlock].insert(genTable[basicBlock].begin(), genTable[basicBlock].end());
				out[basicBlock].insert(temp.begin(), temp.end());

				if (oldout != out[basicBlock]) {
					change = true;
				}
			}
		}
		//printGenTable(&in);

		bool opt_change = false;
		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
			bool block_change = false;
			block_change |= constantFold(basicBlock);
			block_change |= constantProp(basicBlock, &in);
			opt_change |= block_change;	

		}

		if (!opt_change) {
			break;
		}
		ret = true;

	}

	return ret;
}


void gen2(LLVMBasicBlockRef bb, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *genTable, std::set<LLVMValueRef> *iSet, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>> *sucMap, std::set<LLVMValueRef> *gSet) {

	std::set<LLVMValueRef> s;
	std::set<LLVMBasicBlockRef> ss;
	std::set<LLVMValueRef> sCheck;
	(*genTable)[bb] = s;
	(*sucMap)[bb] = ss;

	for(LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction; instruction = LLVMGetNextInstruction(instruction)) {
		(*gSet).insert(instruction);
		LLVMOpcode op = LLVMGetInstructionOpcode(instruction);
		if (op == LLVMStore) {
			LLVMValueRef operand = LLVMGetOperand(instruction, 1);
			if (sCheck.count(operand)) {
				continue;
			} else {
				sCheck.insert(operand);
			}
		} else if (op == LLVMLoad) {
			(*iSet).insert(instruction);
			LLVMValueRef operand = LLVMGetOperand(instruction, 0);
			if (sCheck.count(operand)) {
			// don't add to gen set
				continue;
		    } else {
				(*genTable)[bb].insert(instruction);
		    }
		} else if (LLVMIsATerminatorInst(instruction)) {
		    	unsigned int suc_num = LLVMGetNumSuccessors(instruction);
			for (unsigned int i = 0; i < suc_num; i++) {
				LLVMBasicBlockRef bnew = LLVMGetSuccessor(instruction, i);
				(*sucMap)[bb].insert(bnew);
			}

		}

	}
}


void kill2(std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *genTable, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *killTable, std::set<LLVMValueRef> *iSet) {

	for (auto i : (*genTable)) {
		std::set<LLVMValueRef> s;
		(*killTable)[i.first] = s;
			for (LLVMValueRef instruction = LLVMGetFirstInstruction(i.first); instruction; instruction = LLVMGetNextInstruction(instruction)) {

				LLVMOpcode opcode = LLVMGetInstructionOpcode(instruction);
				if (opcode == LLVMStore) {
					LLVMValueRef storeOpt1 = LLVMGetOperand(instruction, 1);
					const char *tes = LLVMPrintValueToString(storeOpt1);
					for (auto k : (*iSet)) {
						LLVMValueRef loadOpt0 = LLVMGetOperand(k, 0);
						const char *tes2 = LLVMPrintValueToString(loadOpt0);
						if (strcmp(tes, tes2)== 0) {
							(*killTable)[i.first].insert(k);
						}
					}
				}
			}

	}

}

bool storeElim(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *out) {

	bool ret = false;
	//printGenTable(&(*in));
	std::set<LLVMValueRef> R= (*out)[basicBlock];
	std::set<LLVMValueRef> tbd;
	for(LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock); instruction; instruction = LLVMGetNextInstruction(instruction)) {
		LLVMOpcode opcode = LLVMGetInstructionOpcode(instruction);
		bool flag = false;

		if (opcode == LLVMStore) {
			LLVMValueRef op = LLVMGetOperand(instruction, 1);
			const char *tes = LLVMPrintValueToString(op);

			for (auto i : R) {
				LLVMValueRef loadOp = LLVMGetOperand(i, 0);
				const char *tes1 = LLVMPrintValueToString(loadOp);
				if (strcmp(tes1, tes) == 0) {
					//puts("store has ref load ptr");
					flag = true;
					break;
				}
			}
			if (!flag) {
				//puts("Store has no referencded lod ptr");
				ret = true;
				tbd.insert(instruction);
			}
		}
	}

	for(auto i : tbd) {
		LLVMInstructionEraseFromParent(i);
	}

	return ret;


}



bool livevarAnalysis(LLVMValueRef function) {

	bool ret = false;
	int iteration = 0;
	while(true) {
		iteration++;
		//printf("Global opt iteration %d\n", iteration);
		std::unordered_map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>> sucMap;
		std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> genTable;
		std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> killTable;
		std::set<LLVMValueRef> iSet;
		std::set<LLVMValueRef> gSet;

		//std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> in;
		std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> out;

		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
			// gen set
			gen2(basicBlock, &genTable, &iSet, &sucMap, &gSet);
			std::set<LLVMValueRef> s;
			out[basicBlock] = s;
		}

		kill2(&genTable, &killTable, &iSet);
		//puts("******************GENT TABLE****************");
		//printGenTable(&genTable);

		//puts("******************KILL TABLE****************");
		//printGenTable(&killTable);
		// gen map of predecessors
		std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> in(genTable);
		
		bool change = true;
		while (change) {
			
			change = false;

			for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
				out[basicBlock].clear();
				for (auto i : sucMap[basicBlock]) {
					out[basicBlock].insert(in[i].begin(), in[i].end());
				}
				std::set<LLVMValueRef> oldin(in[basicBlock]);
				
				std::set<LLVMValueRef>temp(out[basicBlock]);
				for (auto j : killTable[basicBlock]) {
					temp.erase(j);
				}
				in[basicBlock].clear();
				in[basicBlock].insert(genTable[basicBlock].begin(), genTable[basicBlock].end());
				in[basicBlock].insert(temp.begin(), temp.end());

				if (oldin != in[basicBlock]) {
					change = true;
				}
			}
		}
		printGenTable(&out);

		bool opt_change = false;
		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
			bool block_change = false;
			block_change |= storeElim(basicBlock, &out);
			opt_change |= block_change;	
		}

		if (!opt_change) {
			break;
		}
		ret = true;

	}

	return ret;
}

void walkBasicblocks(LLVMValueRef function) {

	bool loc_change = false;
	bool glob_change = false;
	while(1) {
		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
			loc_change |= common_subexpr(basicBlock);
			loc_change |= deadcodeElim(basicBlock);
			loc_change |= constantFold(basicBlock);
		}
		
		glob_change |= globalOpt(function);
		
		if (!glob_change) {
			break;
		}
		glob_change = false;
	}

	livevarAnalysis(function);
	
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
