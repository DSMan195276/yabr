#ifndef YABR_CONFIG_LEXER_H
#define YABR_CONFIG_LEXER_H

#include <stdio.h>

int yylex(void);
int yylex_destroy(void);

extern FILE *yyin;

#endif
