### Compiler

##### Structure
```txt
├── src/
│   ├── ast/                ; helper files for the astNode
│   │   ├── ast.c
│   │   ├── ast.h
│   │   ├── example.c
│   │   └── Makefile
│   ├── Frontegg/           ; this frontegg contains the parser, semantic analyzer and IR builder
│   │   ├── builder.c
│   │   ├── builder.h
│   │   ├── frontend.l
│   │   ├── frontend.y
│   │   ├── Makefile
│   │   ├── semantic.c
│   │   └── semantic.h
│   ├── Middlegg/           ; contains the optimization logic
│   │   ├── livevar.md
│   │   ├── Makefile
│   │   ├── opt.c
│   │   └── opt.h
│   ├── parser_tests/       ; sample test to test with
│   │   ├── p_bad.c
│   │   ├── p1.c
│   │   ├── p2.c
│   │   ├── p3.c
│   │   ├── p4.c
│   │   └── p5.c
│   ├── entry.c             ; contains the entire flow process of the compiler[frontend -> builder -> middlend ]
│   └── Makefile
└── README.md
```


##### Build/Run
To build, run make in the `src` directory, and the it will automatically compile every part of the project and output an executable `compiler`. Now run the executable with a miniC program `./compiler <mini-c file>` and it will by default output a `test.ll` file and dump the outputs before optimization to the console.
