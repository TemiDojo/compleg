#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "builder.h"
#include "../ast/ast.h"
#include <llvm-c/Core.h> 
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#include <llvm-c/Target.h>
#include <set>

using namespace std;

static size_t unique_id = 0;
static unordered_map<string, string> name_map;
static unordered_map<string, LLVMValueRef> var_map;

static LLVMModuleRef module;
static LLVMBuilderRef builder;
static LLVMValueRef ret_ref;
static LLVMBasicBlockRef retBB;
static LLVMValueRef func_print;
static LLVMValueRef func_read;


char* to_ast_str(string s) {
    return strdup(s.c_str());
}

string gen_unique_name(string var_name, size_t level) {
    return var_name + "." + to_string(level) + "." + to_string(unique_id++);
}

void rename_ast(astNode *root, const char* output_file) {
    if (!root) return;
    unique_id = 0;
    name_map.clear();
    process(root, 0);
    build(root);
    //LLVMDumpModule(module);
    LLVMPrintModuleToFile(module, output_file, NULL);
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
}

void process(astNode *node, size_t level) {
    if (!node) return;

    switch(node->type) {
        case ast_prog:
            process(node->prog.func, level);
            break;

        case ast_func:
            if (node->func.param) process(node->func.param, level);
            process(node->func.body, level + 1);
            break;

        case ast_stmt:
            processStmt(&(node->stmt), level);
            break;

        case ast_var: {
            if (name_map.count(node->var.name)) {
                char* old_name = node->var.name;
                node->var.name = to_ast_str(name_map[old_name]);
                free(old_name); 
            }
            break;
        }
        case ast_rexpr:
            process(node->rexpr.lhs, level);
            process(node->bexpr.rhs, level);
            break;
        case ast_bexpr:
            process(node->bexpr.lhs, level);
            process(node->bexpr.rhs, level);
            break;

        case ast_uexpr:
            process(node->uexpr.expr, level);
            break;

        default: break; 
    }
}

void processStmt(astStmt *stmt, size_t level) {
    if (!stmt) return;

    switch(stmt->type) {
        case ast_decl: {
            string unique = gen_unique_name(stmt->decl.name, level);
            name_map[stmt->decl.name] = unique;
            
            char* old_name = stmt->decl.name;
            stmt->decl.name = to_ast_str(unique);
            free(old_name);
            break;
        }

        case ast_block: {
            vector<astNode*> *slist = stmt->block.stmt_list;
            for (size_t i = 0; i < slist->size(); i++) {
                process((*slist)[i], level);
            }
            break;
        }

        case ast_asgn:
            process(stmt->asgn.lhs, level);
            process(stmt->asgn.rhs, level);
            break;

        case ast_if:
            process(stmt->ifn.cond, level);
            process(stmt->ifn.if_body, level);
            if (stmt->ifn.else_body) process(stmt->ifn.else_body, level);
            break;

        case ast_while:
            process(stmt->whilen.cond, level);
            process(stmt->whilen.body, level);
            break;

        case ast_ret:
            process(stmt->ret.expr, level);
            break;

        case ast_call:
            if (stmt->call.param) process(stmt->call.param, level);
            break;

        default: break;
    }
}

void collectAllNames(astNode* node, set<string>& names) {
    if (!node) return;

    if (node->type == ast_stmt) {
        if (node->stmt.type == ast_decl) names.insert(node->stmt.decl.name);
        else if (node->stmt.type == ast_block) {
            for(auto n : *(node->stmt.block.stmt_list)) collectAllNames(n, names);
        }
        else if(node->stmt.type == ast_if) {
            collectAllNames(node->stmt.ifn.if_body, names);
            if (node->stmt.ifn.else_body) collectAllNames(node->stmt.ifn.else_body, names);
        }
        else if (node->stmt.type == ast_while) collectAllNames(node->stmt.whilen.body, names);
    }
}

void build(astNode *node) {

	switch(node->type) {
		case ast_prog: {
            // Generate a module, set the target architecture
            module = LLVMModuleCreateWithName("module");
            LLVMSetTarget(module, "x86_64-pc-linux-gnu");

            // generate llvm functions without bodies for print and 
            // read extern function declarations
            LLVMTypeRef ret_read = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
            func_read = LLVMAddFunction(module, "read", ret_read);

            LLVMTypeRef param_print[] = {LLVMInt32Type()};
            LLVMTypeRef ret_print = LLVMFunctionType(LLVMVoidType(), param_print, 1, 0);
            func_print = LLVMAddFunction(module, "print", ret_print);



            // visit the function node
            build(node->prog.func);
            break;
       }
        case ast_func: {
            // creating function in a module
            vector<LLVMTypeRef> p_types;
            if (node->func.param) p_types.push_back(LLVMInt32Type());
            LLVMTypeRef func_type = LLVMFunctionType(LLVMInt32Type(), p_types.data(), p_types.size(), 0);
            LLVMValueRef func = LLVMAddFunction(module, node->func.name, func_type);

           // generate a llvm builder
           builder = LLVMCreateBuilder();
           // generate a entry basic block, and let entryBB be the ref to this bb
           LLVMBasicBlockRef entryBB = LLVMAppendBasicBlock(func, "entryBB");
           // create a set with names of all parameters and loca variables
           set<string> param_names;
           set<string> all_names;
           if (node->func.param != NULL) {
               collectAllNames(node->func.param, param_names);
               collectAllNames(node->func.param, all_names);
            }
           collectAllNames(node->func.body, all_names);
           // set the position of builder to the end of entryBB
           LLVMPositionBuilderAtEnd(builder, entryBB);
           // initialize var_map to a new map;
           var_map.clear();
           // for each names in the set created above:
            for(auto& name : all_names) {
                // generate an allocstatement
                // Add names llvmvalueref to alloc statement generated above to var_map
                var_map[name] = LLVMBuildAlloca(builder, LLVMInt32Type(), name.c_str()); 
           }

            // generate an alloc instruction for the return value nad keep the llvmvalue ref, ret_ref,
            // of this instruction available everywhere in your program
            ret_ref = LLVMBuildAlloca(builder, LLVMInt32Type(), "ret_ref");
            // generate a store instruction to store the function parameter(user LLVMGetParam) into
                // the memory location with (alloc instruction) the prameter name in the function ast node.
            if (param_names.size() > 0) {
                for(const std::string& name : param_names) {
                    LLVMBuildStore(builder, LLVMGetParam(func, 0), var_map[name]);
                }
            }

            // generate a return basic block and keep the llvmbasicblockref, retbb, of this basic
            // block available everywhere in your program
            retBB = LLVMAppendBasicBlock(func, "retBB");
            // set the position of the builde to then end of retBB
            LLVMPositionBuilderAtEnd(builder, retBB);
            // Add the folowing LLVM instructions to retbb
                // A load instruction from the ret_ref
                // A return instruction with operand as the load instruction created above
            LLVMValueRef r1_load = LLVMBuildLoad2(builder, LLVMInt32Type(), ret_ref, "val");
            LLVMBuildRet(builder, r1_load);

            // Generate the IR for the function body by call genIRstmt subroutine given below: pass entryBB as a param
            // to genIRStmt and let the exitBB be the return value of genIRStmt call
            LLVMBasicBlockRef exitBB = genIRStmt(node->func.body, entryBB);
            // get the terminator instruction of exitBB basic block
            // if the termina is NULL( there is no return statement at the end of the function body)
            if (LLVMGetBasicBlockTerminator(exitBB) == NULL) {
                LLVMPositionBuilderAtEnd(builder, exitBB);
                LLVMBuildBr(builder, retBB);
            }
            break;

       }

	}

}

LLVMBasicBlockRef genIRStmt(astNode *node, LLVMBasicBlockRef startBB) {

    if (!node) return startBB;
    LLVMPositionBuilderAtEnd(builder, startBB);
    LLVMValueRef func = LLVMGetBasicBlockParent(startBB);

    if (node->type == ast_stmt) {
        astStmt *s = &(node->stmt);
        switch(s->type) {
            case ast_asgn: {
                LLVMValueRef rhs = genIRExpr(s->asgn.rhs);
                LLVMBuildStore(builder, rhs, var_map[s->asgn.lhs->var.name]);
                return startBB;
           }
            case ast_call: {
                LLVMValueRef val = genIRExpr(s->call.param);
                LLVMTypeRef param_print[] = {LLVMInt32Type()};
                LLVMTypeRef ret_print = LLVMFunctionType(LLVMVoidType(), param_print, 1, 0);
                LLVMBuildCall2(builder, ret_print, func_print, &val, 1, "");
                return startBB;
           }
            case ast_while: {
                
                LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(func, "condBB");
                LLVMBuildBr(builder, condBB);

                LLVMPositionBuilderAtEnd(builder, condBB);
                LLVMValueRef condVal = genIRExpr(s->whilen.cond);
                LLVMBasicBlockRef trueBB = LLVMAppendBasicBlock(func, "trueBB");
                LLVMBasicBlockRef falseBB = LLVMAppendBasicBlock(func, "falseBB");

                LLVMBuildCondBr(builder, condVal, trueBB, falseBB);

                LLVMBasicBlockRef trueXitBB = genIRStmt(s->whilen.body, trueBB);
                LLVMPositionBuilderAtEnd(builder, trueXitBB);
                LLVMBuildBr(builder, condBB);
                return falseBB;
            }
            case ast_if: {
                LLVMValueRef condVal = genIRExpr(s->ifn.cond);
                LLVMBasicBlockRef trueBB = LLVMAppendBasicBlock(func, "trueBB");
                LLVMBasicBlockRef falseBB = LLVMAppendBasicBlock(func, "falseBB");
                LLVMBuildCondBr(builder, condVal, trueBB, falseBB);
                if (s->ifn.else_body == NULL) {
                    LLVMBasicBlockRef ifExitBB = genIRStmt(s->ifn.if_body, trueBB);
                    LLVMPositionBuilderAtEnd(builder, ifExitBB);
                    LLVMBuildBr(builder, falseBB);
                    return falseBB;
                } else {
                    LLVMBasicBlockRef ifExitBB = genIRStmt(s->ifn.if_body, trueBB);
                    LLVMBasicBlockRef elseExitBB = genIRStmt(s->ifn.else_body, falseBB);
                    LLVMBasicBlockRef endBB = LLVMAppendBasicBlock(func, "endBB");

                    LLVMPositionBuilderAtEnd(builder, ifExitBB); LLVMBuildBr(builder, endBB);
                    LLVMPositionBuilderAtEnd(builder, elseExitBB); LLVMBuildBr(builder, endBB);
                    return endBB;
                }
             }
            case ast_block: {
                LLVMBasicBlockRef prevBB = startBB;
                for (auto n : *(s->block.stmt_list)) {
                    prevBB = genIRStmt(n, prevBB);
                }
                return prevBB;
            }
            case ast_ret: {
                LLVMValueRef retval = genIRExpr(s->ret.expr);
                LLVMBuildStore(builder, retval, ret_ref);
                LLVMBuildBr(builder, retBB);
                return LLVMAppendBasicBlock(func, "afterRetBB");
          }
            default: return startBB;

        }
    }
    return startBB;
}

LLVMValueRef genIRExpr(astNode *node) {

    if (!node) return NULL;
    switch(node->type) {
        case ast_cnst:
            return LLVMConstInt(LLVMInt32Type(), node->cnst.value, 0);
        case ast_var:
            return LLVMBuildLoad2(builder, LLVMInt32Type(), var_map[node->var.name], "");
        case ast_uexpr: {
            LLVMValueRef v = genIRExpr(node->uexpr.expr);
            return LLVMBuildSub(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), v, "");
        }
        case ast_bexpr: {
            LLVMValueRef l = genIRExpr(node->bexpr.lhs);
            LLVMValueRef r = genIRExpr(node->bexpr.rhs);
            if (node->bexpr.op == add) return LLVMBuildAdd(builder, l, r, "");
            if (node->bexpr.op == sub) return LLVMBuildSub(builder, l, r, "");
            if (node->bexpr.op == mul) return LLVMBuildMul(builder, l, r, "");
            return NULL;
        }
        case ast_rexpr: {
            LLVMValueRef l = genIRExpr(node->rexpr.lhs);
            LLVMValueRef r = genIRExpr(node->rexpr.rhs);
            LLVMIntPredicate p;
            if (node->rexpr.op == lt) p = LLVMIntSLT;
            else if (node->rexpr.op == gt) p = LLVMIntSGT;
            else if (node->rexpr.op == eq) p = LLVMIntEQ;
            else if (node->rexpr.op == neq) p = LLVMIntNE;
            else if (node->rexpr.op == le) p = LLVMIntSLE;
            else p = LLVMIntSGE;
            return LLVMBuildICmp(builder, p, l, r, "");
        }
        case ast_stmt: {
            if (node->stmt.type == ast_call) {
                LLVMTypeRef read_type = LLVMFunctionType(LLVMInt32Type(), NULL, 0 , 0);
                return LLVMBuildCall2(builder, read_type, func_read, NULL, 0, "");
            }
            return NULL;
       }
        default: return NULL;
    }


}
