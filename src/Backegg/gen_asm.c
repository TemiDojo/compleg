#include "gen_asm.h"

std::map<LLVMValueRef, int> reg_map;
std::map<LLVMValueRef, int> inst_index;
std::map<LLVMValueRef, std::pair<int, int>> live_range;

using namespace std;

void compute_liveness(LLVMBasicBlockRef b) {
    inst_index.clear();
    live_range.clear();
    int index = 0;

    for(LLVMValueRef i = LLVMGetFirstInstruction(b); i; i = LLVMGetNextInstruction(i)) {
        if(LLVMIsAAllocaInst(i)) continue;

        live_range[i] = {inst_index[i], inst_index[i]};
        int numOps = LLVMGetNumOperands(i);
        for(int j = 0; j < numOps; j++) {
            LLVMValueRef op = LLVMGetOperand(i, j);
            if (inst_index.count(op)) {
                live_range[op].second = inst_index[i];
            }
        }

    }
}

void get_inst_index(LLVMBasicBlockRef b) {
    inst_index.clear();
    int index = 0;

    for(LLVMValueRef i = LLVMGetFirstInstruction(b); i; i = LLVMGetNextInstruction(i)) {
        if (LLVMIsAAllocaInst(i)) continue;
        inst_index[i] = index++;
    }
}

void freeRegs(LLVMValueRef i, std::set<int> &available_regs) {
    int numOps = LLVMGetNumOperands(i);
    for(int j = 0; j < numOps; j++) {
        LLVMValueRef op = LLVMGetOperand(i, j);
        // if live nrage of any operand of instr ends
        if (live_range.count(op) && live_range[op].second == inst_index[i]) {
            // and it has physical register P assigned to it
            if (reg_map.count(op) && reg_map[op] != -1) {
                // the nadd P to availabel set of registers
                available_regs.insert(reg_map[op]);
            }
        }

    }
}

LLVMValueRef find_spill(LLVMValueRef i) {
    
    vector<LLVMValueRef> sorted_list;
    for (auto const& [val, range] : live_range) {
        sorted_list.push_back(val);
    }

    sort(sorted_list.begin(), sorted_list.end(), [&](LLVMValueRef a, LLVMValueRef b) {
        return live_range[a].second > live_range[b].second;
    });

    int instr_idx = inst_index[i];
    for (LLVMValueRef V : sorted_list) {
        pair<int, int> rangeV = live_range[V];
        
        if (rangeV.first <= instr_idx && rangeV.second >= instr_idx) {
            
            if (reg_map.count(V) && reg_map[V] != -1) {
                return V;
            }
        }
    }

    return NULL;
}

void reg_alloc(LLVMValueRef func) {

    // for each basic block B in the function
    for(LLVMBasicBlockRef b = LLVMGetFirstBasicBlock(func); b; b = LLVMGetNextBasicBlock(b)) {
        // initialize the set of available physical registers to (ebx, ecx, edx)
        std::set<int> available_regs = {0, 1, 2};
        // get inst_index for basic block B
        get_inst_index(b);
        // get live_range and by calling compute_liveness on the B
        compute_liveness(b);
    
        // for each inst in the basic block
        for (LLVMValueRef i = LLVMGetFirstInstruction(b); i; i = LLVMGetNextInstruction(i)) {
            // ingnore instr if it is an alloc instruction
            if (LLVMIsAAllocaInst(i)) continue;

                // if instr is an instruction that does not have a result(e.g. store, branch, call that doesn't return a value)
                if (LLVMIsAStoreInst(i) || LLVMIsABranchInst(i) || (LLVMIsACallInst(i) && LLVMGetTypeKind(LLVMTypeOf(i)) == LLVMVoidTypeKind)) {
                    freeRegs(i, available_regs);
                }
                // if the following about instr hold: a) is of type add/sub/mul...
                LLVMOpcode opc = LLVMGetInstructionOpcode(i);
                if (opc == LLVMAdd || opc == LLVMSub || opc == LLVMMul) {
                    LLVMValueRef op1 = LLVMGetOperand(i, 0);
                    if (reg_map.count(op1) && reg_map[op1] != -1 && live_range[op1].second == inst_index[i]) {
                        // Add the entry instru ->R to reg_map
                        reg_map[i] = reg_map[op1];
                        // if live rangeof second operand of i ends, and it has a physicla register P assigned...
                        LLVMValueRef op2 = LLVMGetOperand(i, 1);
                        if (live_range.count(op1) && live_range[op2].second == inst_index[i]) {
                            if (reg_map.count(op2) && reg_map[op2] != -1) available_regs.insert(reg_map[op2]);
                        }
                        continue;
                    }
                }

                // if a physical register R is avaible:
                if (!available_regs.empty()) {
                    int R = *available_regs.begin();
                    reg_map[i] = R; // Add the entry instr-> R to regs_map
                    available_regs.erase(R);    // remove R from the available pool
                    freeRegs(i, available_regs);
                } 
                // if a physical register is not available
                else {
                    LLVMValueRef V = find_spill(i);
                    // if V has more uses that instr
                    if (V != NULL) {
                        if (live_range[i].second > live_range[V].second) {
                            reg_map[i] = -1;
                        } else {
                            int R = reg_map[V];
                            reg_map[i] = R;
                            reg_map[V] = -1;
                        }
                    }
                    freeRegs(i, available_regs);

                }
                        
        }

    }
}
