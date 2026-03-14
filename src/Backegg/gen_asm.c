#include "gen_asm.h"
#include <string.h>

std::map<LLVMValueRef, int> reg_map;
std::map<LLVMValueRef, int> inst_index;
std::map<LLVMValueRef, std::pair<int, int>> live_range;
std::map<LLVMBasicBlockRef, std::string> bb_labels;
std::map<LLVMValueRef, int> offset_map;
int localMem = 4;

using namespace std;

std::string getRegName(int regIdx) {
    if (regIdx == 0) return "%ebx";
    if (regIdx == 1) return "%ecx";
    if (regIdx == 2) return "%edx";
    return "%eax";
}

void compute_liveness(LLVMBasicBlockRef b) {
    live_range.clear();

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


void createBBLabels(LLVMValueRef func) {
    int labelCount = 0;
    for (LLVMBasicBlockRef b = LLVMGetFirstBasicBlock(func); b; b = LLVMGetNextBasicBlock(b)) {
        char* label = (char *)malloc(16);
        sprintf(label, ".L%d", labelCount++);
        bb_labels[b] = label;
    }
}

void printDirectives(FILE* out, const char* funcName) {
    fprintf(out, "\t.text\n");
    fprintf(out, "\t.globl %s\n", funcName);
    fprintf(out, "\t.type %s, @function\n", funcName);
    fprintf(out, "%s:\n", funcName);
}

void printFunctionEnd(FILE *out) {
    fprintf(out, "\tleave\n");
    fprintf(out, "\tret\n");
}

void getOffsetMap(LLVMValueRef func) {
    // initialize localMem to 4
    localMem = 4;
    // if the function has param....
    LLVMValueRef param = LLVMGetParam(func, 0);
    if (param) {
        offset_map[param] = 8;
    }
    // for each basic block in the function
    for (LLVMBasicBlockRef b = LLVMGetFirstBasicBlock(func); b; b = LLVMGetNextBasicBlock(b)) {
        // for each instruction in the basic block
        for (LLVMValueRef i = LLVMGetFirstInstruction(b); i; i = LLVMGetNextInstruction(i)) {
            // if instr is an alloc instruction
            if (LLVMIsAAllocaInst(i)) {
                // Add 4 to the localMem
                localMem += 4;
                // add instr as the key with the associated value as ... to offset_map
                offset_map[i] = -localMem;
            }
            // if instr is a store instruction
            else if(LLVMIsAStoreInst(i)) {
                LLVMValueRef Op1 = LLVMGetOperand(i, 0);
                LLVMValueRef Op2 = LLVMGetOperand(i, 1);
                // if the first operand of the store instruction is equal to the function parameter
                if (Op1 == param) {
                    // get the value associated with the first operand in offst_map. let thsi bne x
                    int x = offset_map[Op1];
                    // change the value associated with the second operand to x
                    offset_map[Op2] = x;
                }
                // if first operand of the store instruction is not equal to the function parameter and is not a constant
                else if (!LLVMIsAConstant(Op1)) {
                    // get the value associated with the second operation in offset_map. let this be x
                    int x = offset_map[Op2];
                    // ad the first operand as th key with the associated value as x in offset_map
                    offset_map[Op1] = x;
                }
            }
            // if instr is a load instruction
            else if (LLVMIsALoadInst(i)) {
                // get the value associated with the first operand. let this be x
                LLVMValueRef Op1 = LLVMGetOperand(i, 0);
                int x = offset_map[Op1];
                // Add instr as the key with the associated value as x in the offset_map
                offset_map[i] = x;
            }
        }
    }
} 


void generateAssembly(LLVMModuleRef Mod, FILE* out) {
    // for each function defined in your module
    for (LLVMValueRef func = LLVMGetFirstFunction(Mod); func; func = LLVMGetNextFunction(func)) {
        if (LLVMCountBasicBlocks(func) == 0) continue; 

        // createBBLabels
        createBBLabels(func);

        // call printDirectives (.text, .globl, .type, function label)
        printDirectives(out, LLVMGetValueName(func));

        // call getOffsetMap to initialize offset_map and localMem
        getOffsetMap(func);

        // emit pushl %ebp
        fprintf(out, "\tpushl %%ebp\n");
        // emit movl %esp, %ebp
        fprintf(out, "\tmovl %%esp, %%ebp\n");
        // emit subl localMem, %esp
        fprintf(out, "\tsubl $%d, %%esp\n", localMem);
        // emit pushl %ebx
        fprintf(out, "\tpushl %%ebx\n");

        // for each basicBlock BB in the function
        for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func); bb; bb = LLVMGetNextBasicBlock(bb)) {
            fprintf(out, "%s:\n", bb_labels[bb].c_str());

            // for each instruction Instr in BB
            for (LLVMValueRef Instr = LLVMGetFirstInstruction(bb); Instr; Instr = LLVMGetNextInstruction(Instr)) {
                LLVMOpcode opc = LLVMGetInstructionOpcode(Instr);

                if (opc == LLVMRet) {
                    LLVMValueRef A = LLVMGetOperand(Instr, 0);
                    if (LLVMIsAConstantInt(A)) {
                        fprintf(out, "\tmovl $%ld, %%eax\n", LLVMConstIntGetSExtValue(A));
                    }
                    else if (reg_map[A] == -1)  {
                        fprintf(out, "\tmovl %d(%%ebp), %%eax\n", offset_map[A]);
                    }
                    else {
                        fprintf(out, "\tmovl %s, %%eax\n", getRegName(reg_map[A]).c_str());
                    }
                    
                    fprintf(out, "\tpopl %%ebx\n");
                    printFunctionEnd(out);
                }

                else if (opc == LLVMLoad) {
                    if (reg_map[Instr] != -1) {
                        int c = offset_map[LLVMGetOperand(Instr, 0)];
                        fprintf(out, "\tmovl %d(%%ebp), %s\n", c, getRegName(reg_map[Instr]).c_str());
                    }
                }

                else if (opc == LLVMStore) {
                    LLVMValueRef A = LLVMGetOperand(Instr, 0);
                    LLVMValueRef b_ptr = LLVMGetOperand(Instr, 1);
                    if (A == LLVMGetParam(func, 0)) continue; 

                    if (LLVMIsAConstantInt(A)) {
                        fprintf(out, "\tmovl $%ld, %d(%%ebp)\n", LLVMConstIntGetSExtValue(A), offset_map[b_ptr]);
                    } else if (reg_map[A] != -1) {
                        fprintf(out, "\tmovl %s, %d(%%ebp)\n", getRegName(reg_map[A]).c_str(), offset_map[b_ptr]);
                    } else {
                        fprintf(out, "\tmovl %d(%%ebp), %%eax\n", offset_map[A]);
                        fprintf(out, "\tmovl %%eax, %d(%%ebp)\n", offset_map[b_ptr]);
                    }
                }

                else if (opc == LLVMCall) {
                    fprintf(out, "\tpushl %%ecx\n\tpushl %%edx\n");
                    int numArgs = LLVMGetNumArgOperands(Instr);
                    if (numArgs > 0) {
                        LLVMValueRef P = LLVMGetArgOperand(Instr, 0);
                        if (LLVMIsAConstantInt(P)) fprintf(out, "\tpushl $%ld\n", LLVMConstIntGetSExtValue(P));
                        else if (reg_map[P] != -1) fprintf(out, "\tpushl %s\n", getRegName(reg_map[P]).c_str());
                        else fprintf(out, "\tpushl %d(%%ebp)\n", offset_map[P]);
                    }
                    fprintf(out, "\tcall %s\n", LLVMGetValueName(LLVMGetCalledValue(Instr)));
                    if (numArgs > 0) fprintf(out, "\taddl $4, %%esp\n");
                    fprintf(out, "\tpopl %%edx\n\tpopl %%ecx\n");
                    
                    if (LLVMGetTypeKind(LLVMTypeOf(Instr)) != LLVMVoidTypeKind) {
                        if (reg_map[Instr] != -1) fprintf(out, "\tmovl %%eax, %s\n", getRegName(reg_map[Instr]).c_str());
                        else fprintf(out, "\tmovl %%eax, %d(%%ebp)\n", offset_map[Instr]);
                    }
                }

                else if (opc == LLVMBr) {
                    if (!LLVMIsConditional(Instr)) {
                        fprintf(out, "\tjmp %s\n", bb_labels[LLVMValueAsBasicBlock(LLVMGetOperand(Instr, 0))].c_str());
                    } else {
                        string L1 = bb_labels[LLVMValueAsBasicBlock(LLVMGetOperand(Instr, 2))];
                        string L2 = bb_labels[LLVMValueAsBasicBlock(LLVMGetOperand(Instr, 1))];
                        LLVMIntPredicate T = LLVMGetICmpPredicate(LLVMGetOperand(Instr, 0));
                        
                        if (T == LLVMIntSLT) fprintf(out, "\tjl %s\n", L1.c_str());
                        else if (T == LLVMIntSGT) fprintf(out, "\tjg %s\n", L1.c_str());
                        else if (T == LLVMIntSLE) fprintf(out, "\tjle %s\n", L1.c_str());
                        else if (T == LLVMIntSGE) fprintf(out, "\tjge %s\n", L1.c_str());
                        else if (T == LLVMIntEQ) fprintf(out, "\tje %s\n", L1.c_str());
                        else if (T == LLVMIntNE) fprintf(out, "\tjne %s\n", L1.c_str());
                        fprintf(out, "\tjmp %s\n", L2.c_str());
                    }
                }

                else if (opc == LLVMAdd || opc == LLVMSub || opc == LLVMMul || opc == LLVMICmp) {
                    LLVMValueRef A = LLVMGetOperand(Instr, 0);
                    LLVMValueRef B = LLVMGetOperand(Instr, 1);
                    string R = (reg_map[Instr] != -1) ? getRegName(reg_map[Instr]) : "%eax";
                    
                    if (LLVMIsAConstantInt(A)) fprintf(out, "\tmovl $%ld, %s\n", LLVMConstIntGetSExtValue(A), R.c_str());
                    else if (reg_map[A] != -1) {
                        if (getRegName(reg_map[A]) != R) fprintf(out, "\tmovl %s, %s\n", getRegName(reg_map[A]).c_str(), R.c_str());
                    } else fprintf(out, "\tmovl %d(%%ebp), %s\n", offset_map[A], R.c_str());

                    string asmOp = (opc == LLVMAdd) ? "addl" : (opc == LLVMSub) ? "subl" : (opc == LLVMMul) ? "imull" : "cmpl";
                    
                    if (LLVMIsAConstantInt(B)) fprintf(out, "\t%s $%ld, %s\n", asmOp.c_str(), LLVMConstIntGetSExtValue(B), R.c_str());
                    else if (reg_map[B] != -1) fprintf(out, "\t%s %s, %s\n", asmOp.c_str(), getRegName(reg_map[B]).c_str(), R.c_str());
                    else fprintf(out, "\t%s %d(%%ebp), %s\n", asmOp.c_str(), offset_map[B], R.c_str());

                    if (opc != LLVMICmp && reg_map[Instr] == -1) 
                        fprintf(out, "\tmovl %%eax, %d(%%ebp)\n", offset_map[Instr]);
                }
            }
        }
    }
}


void codegen(LLVMModuleRef *Mod, const char* filename) {

    LLVMModuleRef m = *Mod;

    if (filename == NULL) {
        filename = strdup("out.s");
    }

    FILE* out = fopen(filename, "w");
    if (!out) {
        printf("Error: could not open file %s\n", filename);
        return;
    }

    for (LLVMValueRef func = LLVMGetFirstFunction(m); func; func = LLVMGetNextFunction(func)) {
        if (LLVMCountBasicBlocks(func) == 0) continue;
        createBBLabels(func);
        reg_alloc(func);
        getOffsetMap(func);
    }

    generateAssembly(m, out);

    fclose(out);

}

