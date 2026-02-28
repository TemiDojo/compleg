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


void process(astNode *node, size_t level);
void processStmt(astStmt *stmt, size_t level);
char* to_ast_str(string s);
string gen_unique_name(string var_name, size_t level);
void rename_ast(astNode *root, const char* output_file);
void build(astNode *node);
LLVMBasicBlockRef genIRStmt(astNode *node, LLVMBasicBlockRef startBB);
LLVMValueRef genIRExpr(astNode *node);
