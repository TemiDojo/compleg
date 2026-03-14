#include <llvm-c/Types.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Core.h>
#include <unordered_map>
#include <vector>
#include <cstddef>
#include <set>
#include <map>
#include <algorithm>
#include <cstdio>
#include <string>

void reg_alloc(LLVMValueRef func);
void compute_liveness(LLVMBasicBlockRef b);
void get_inst_index(LLVMBasicBlockRef b);
void freeRegs(LLVMValueRef i, std::set<int> &available_regs);
LLVMValueRef find_spill(LLVMValueRef i);

std::string getRegName(int regIdx);
void createBBLabels(LLVMValueRef func);
void printDirectives(FILE* out, const char* funcName);
void printFunctionEnd(FILE* out);
void getOffsetMap(LLVMValueRef func);
void codegen(LLVMModuleRef *Mod, const char* filename);
void generateAssembly(LLVMModuleRef Mod, FILE *out);
