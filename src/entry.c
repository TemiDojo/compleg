#include <stdio.h>
#include "./ast/ast.h"
#include "./Frontegg/y.tab.h"
#include "./Frontegg/semantic.h"
#include "./Frontegg/builder.h"

extern astNode *root;
extern FILE *yyin;
extern int yyparse(astNode **root);

int main(int argc, char **argv) {

	astNode *root = NULL;
	if (argc == 2) {
		yyin = fopen(argv[1], "r");
		if (yyin == NULL) {
			fprintf(stderr, "file open error\n");
			return 1;
		}
	}
	
	yyparse(&root);

	if (root == NULL) {
		printf("Error: root is NULL\n");
		return -1;
	}

	//printNode(root);

	// semantic analysis
	semantic_analysis(root);
	// IR builder
	rename_ast(root);	


}
