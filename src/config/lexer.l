
%{
#include <string.h>
#include <stdio.h>

#include "status.h"
#include "render.h"
#include "status_desc.h"

#include "lexer.h"
#include "parser.h"

#define YY_NO_INPUT
#define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno;
%}

%option noyywrap
%option yylineno

%x string
%x comment

%%

\n { } /* New line  - for lineno */

[ \t]+ /* White space */

("#"|"//")[^\n]* /* One line comment */

"{" { return '{'; }
"}" { return '}'; }
"=" { return '='; }
"," { return ','; }
";" { return ';'; }
"[" { return '['; }
"]" { return ']'; }

[0-9]+ {
    yylval.ival = atoi(yytext);
    return TOK_INTEGER;
}

"status" { return TOK_STATUS; }
"right" { return TOK_RIGHT; }
"center" { return TOK_CENTER; }
"var" { return TOK_VAR; }

(0x)?[a-fA-F0-9]{8} {
    sscanf(yytext, "%x", &yylval.color);
    return TOK_COLOR;
}

[a-zA-Z0-9_\-]+ {
    yylval.str = strdup(yytext);
    return TOK_IDENT;
}

"\"" {
    BEGIN(string);
}

<string>[^\"]* {
    yylval.str = strdup(yytext);
    return TOK_STRING;
}

<string>"\"" {
    BEGIN(INITIAL);
}

"/*" { BEGIN(comment); }
<comment>"*/" { BEGIN(INITIAL); }
<comment>\n { }
<comment>. { }

<<EOF>> {
    return TOK_EOF;
}

. {
    printf("Unexpected token %s\n", yytext);
    return TOK_ERR;
}

%%

/* Get rid of error about unused yyunput */
void fake_func(void)
{
    yyunput(0, 0);
}

