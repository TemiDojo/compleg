
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

LLVMModuleRef createLLVMModel(char * filename);
void printMap(std::unordered_map<std::string, LLVMValueRef> *m);
void printMap2(std::unordered_map<std::string, std::vector<LLVMValueRef>> *m);
bool common_subexpr(LLVMBasicBlockRef bb); 
bool isValid(LLVMOpcode opcode);
bool deadcodeElim(LLVMBasicBlockRef bb);
bool constantFold(LLVMBasicBlockRef bb);
void printGenTable(std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *m);
void gen(LLVMBasicBlockRef bb, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *genTable, std::set<LLVMValueRef> *iSet, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>> *predMap, std::set<LLVMValueRef> *gSet);
void kill(std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *genTable, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *killTable, std::set<LLVMValueRef> *iSet);
bool constantProp(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *in);
bool globalOpt(LLVMValueRef function);
void gen2(LLVMBasicBlockRef bb, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *genTable, std::set<LLVMValueRef> *iSet, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>> *sucMap, std::set<LLVMValueRef> *gSet);
void kill2(std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *genTable, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *killTable, std::set<LLVMValueRef> *iSet);
bool storeElim(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> *out);
bool livevarAnalysis(LLVMValueRef function);
void walkBasicblocks(LLVMValueRef function);
void walkFunctions(LLVMModuleRef module);
