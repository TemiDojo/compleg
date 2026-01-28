

#include "../ast/ast.h"



int semantic_analysis(astNode *rootPtr);
void traverseRoot(astNode *node, vector<char *> *symbol_table);
void traverseStmt(astStmt *stmt, vector<char *> *symbol_table);
int stack_lookup(char *symbol);
int symTab_lookup(char *symbol, vector<char *> sym_table);








