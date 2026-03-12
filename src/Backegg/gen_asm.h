#include <llvm-c/Types.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Core.h>
#include <unordered_map>
#include <vector>
#include <cstddef>
#include <set>
#include <map>
#include <algorithm>

void reg_alloc(LLVMValueRef func);
void compute_liveness(LLVMBasicBlockRef b);
void get_inst_index(LLVMBasicBlockRef b);
void freeRegs(LLVMValueRef i, std::set<int> &available_regs);
LLVMValueRef find_spill(LLVMValueRef i);

void createBBLabels();
void printDirectives();
void getOffsetMap(LLVMValueRef func);
