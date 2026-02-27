#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "../ast/ast.h"
#include <llvm-c/Core.h> 
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>

using namespace std;

static size_t unique_id = 0;
static unordered_map<string, string> name_map;
static unordered_map<string, LLVMValueRef> var_map;

static LLVMModuleRef module;
static LLVMBuildRef builder;
static LLVMValueRef ret_ref;
static LLVMBasicBlockRef retBB;
static LLVMValueRef func_print;
static LLVMValueRef func_read;

void process(astNode *node, size_t level);
void processStmt(astStmt *stmt, size_t level);

char* to_ast_str(string s) {
    return strdup(s.c_str());
}

string gen_unique_name(string var_name, size_t level) {
    return var_name + "." + to_string(level) + "." + to_string(unique_id++);
}

void rename_ast(astNode *root) {
    if (!root) return;
    unique_id = 0;
    name_map.clear();
    process(root, 0);
    printNode(root);
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


void build(astNode *root) {

	switch(node->type) {
		case ast_prog: {
            // Generate a module, set the target architecture
            module = LLVMModuleCreateWithName("module");
            LLVMSetTarget(module, "x86_64-pc-linux-gnu");

            LLVMTypeRef param_read[] = {};
            LLVMTypeRef ret_read = LLVMFunctionType(LLVMInt32Type(), param_read, 1, 0);
            func_read = LLVMAddFunction(module, "read", ret_read);

            LLVMTypeRef param_print[] = {};
            LLVMTypeRef ret_print = LLVMFunctionType(LLVMInt32Type(), param_print, 1, 0);
            func_print = LLVMAddFunction(module, "print", ret_print);

            // visit the function node
            build(node->prog.func);
            break;
                       }
        case ast_func: {
           // generate a llvm builder
           // generate a entry basic block, and let entryBB be the ref to this bb
           // create a set with names of all parameteers nad loca variables
           // set the position of builder to the end of entryBB
           builder = LLVMCreateBuilder();
           var_map.clear();

           vector<LLVMTypeRef> param_t;
           vector<string> param_names;
           astNode* p = node->func.param;
                       }

	}

}
