#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "semantic.h"

std::vector<vector<char *> *> symbol_stack;
int semantic_analysis(astNode *rootPtr) {

    //printNode(rootPtr);
    // loop throught the parse tree and create symbol table
    // declarations can only be found in functions and in statments
    traverseRoot(rootPtr, NULL);

    printf("SUCCESS : semantic analysis\n");

    return 0;


}

void traverseRoot(astNode *node, vector<char *> *symbol_table) {

    switch(node->type) {
        case ast_prog: {
                           traverseRoot(node->prog.func, NULL);
                           break;
                       }

        case ast_func: {
                            std::vector<char *> *sym_table = new vector<char *>();                            
                            symbol_stack.push_back(sym_table);

                            traverseRoot(node->func.param, sym_table);
                            traverseRoot(node->func.body, NULL);
                            symbol_stack.pop_back();  
                            break;
                        }
        case ast_stmt: {
                            astStmt stmt = node->stmt;
                            traverseStmt(&stmt, symbol_table); 
                            break;
                       }
        case ast_var:   {
                            // check if it is valid
                            if (stack_lookup(node->var.name) < 0) {
                                printf("Error: variable {%s} not declared\n", node->var.name); 
                                exit(-1);
                            }
                            break;
                        }
        case ast_bexpr: {
                            traverseRoot(node->bexpr.lhs, symbol_table);
                            traverseRoot(node->bexpr.rhs, symbol_table);
                            break;
                         }
        case ast_rexpr: {
                            traverseRoot(node->rexpr.lhs, symbol_table);
                            traverseRoot(node->rexpr.rhs, symbol_table);
                            break;
                        }
        case ast_cnst:  {
                            break;
                        }
        case ast_uexpr: {
                            break;
                        }
        case ast_extern: 
                        {
                            break;
                        }
        default: {
                     printf("Incorrect node type\n");
                     exit(-1);
                 }

    }

}


void traverseStmt(astStmt *stmt, vector<char *> *symbol_table) {

    assert(stmt != NULL);

    switch(stmt->type) {
        case ast_block: {
                            // create new sym_table
                            vector<char *> *sym_table = new vector<char *>();
                            symbol_stack.push_back(sym_table);
                            vector<astNode*> slist = *(stmt->block.stmt_list);
                            vector<astNode*>::iterator it = slist.begin();
                            while( it != slist.end()) {
                                traverseRoot(*it, sym_table);
                                //symbol_stack.pop_back();
                                it++;
                            }
                            symbol_stack.pop_back();
                            break;
                        }
        case ast_decl: {
                           // we can only have one declaration
                            if (symTab_lookup(stmt->decl.name, *symbol_table) == 0){
                                printf("Error: can only have one declaration in a scope\n");
                                exit(-1);
                            } else {
                                // we add to the symbol table
                                (*symbol_table).push_back(stmt->decl.name);
                            }
                            break;

                       }
        case ast_asgn: {
                            traverseRoot(stmt->asgn.lhs, symbol_table);
                            traverseRoot(stmt->asgn.rhs, symbol_table);
                            break;

                       }
        case ast_while: {
                            traverseRoot(stmt->whilen.cond, symbol_table);
                            traverseRoot(stmt->whilen.body, symbol_table);
                            break;
                        }
        case ast_if:    {
                            traverseRoot(stmt->ifn.cond, symbol_table);
                            traverseRoot(stmt->ifn.if_body, symbol_table);
                            if (stmt->ifn.else_body != NULL) {
                                traverseRoot(stmt->ifn.else_body, symbol_table);
                            }
                            break;
                        }
        case ast_call:  {
                            if (stmt->call.param != NULL) {
                                traverseRoot(stmt->call.param, symbol_table);
                            }
                            break;
                        }
        case ast_ret: {
                          traverseRoot(stmt->ret.expr, symbol_table);
                          break;
                      }
        default:   {
                            printf("Incorrect node type\n");
                            exit(-1);
                        }

    }

}

int symTab_lookup(char *symbol, vector<char *> sym_table) {
    vector<char *>::iterator it = sym_table.begin();
    while(it != sym_table.end()) {
        char *sym = *it;
        if (strcmp(sym, symbol) == 0) {
            return 0;
        }
        it++;
    }
    return -1;

}

int stack_lookup(char *symbol) {
    vector<vector<char *> *>::iterator it = symbol_stack.end();
    while (it != symbol_stack.begin()) {
        it--;
        vector<char *> *sym_table = *it;
        if (symTab_lookup(symbol, *sym_table) == 0) {
            return 0;
        }
    }

    return -1;
}
