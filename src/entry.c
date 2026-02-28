#include <stdio.h>
#include "./ast/ast.h"
#include "./Frontegg/y.tab.h"
#include "./Frontegg/semantic.h"
#include "./Frontegg/builder.h"
#include "./Middlegg/opt.h"

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
    puts("Semantic Analysis");
	semantic_analysis(root);
    puts("Done");
	// IR builder
    const char * outputfile = "test.ll";
    puts("IR Builder");
	rename_ast(root, outputfile);	
    puts("Done");
    puts("Optimizations");
    beginOpt(outputfile);
    puts("Done");


}
