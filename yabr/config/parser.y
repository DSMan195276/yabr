%{
#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "list.h"
#include "status.h"
#include "lexer.h"

#define IN_PARSER
#include "parser.h"

void yyerror(struct parse_state *state, const char *str);

#define YYERROR_VERBOSE
%}

%union {
    char *str;
    int ival;
    struct status *status;
}

%parse-param { struct parse_state *state }
%locations

%token <str> TOK_IDENT
%token <str> TOK_STRING
%token TOK_STATUS
%token TOK_CENTER
%token TOK_RIGHT
%token <ival> TOK_INTEGER
%token TOK_EOF TOK_ERR

%type <status> status

%start config_file

%%

config_file:
    block_list {
        printf("block_list...\n");
    }
    ;

block_list:
    block
    | block_list ',' block
    | block_list TOK_ERR {
        YYABORT;
    }
    | block_list TOK_EOF {
        YYACCEPT;
    }
    ;

status_property:
    TOK_IDENT '=' TOK_STRING {
        if (status_config_set_str(state->items, $1, $3) == -1) {
            fprintf(stderr, "%s: %d: Status \"%s\" does not have string item \"%s\"\n",
                yabr_config.config_file, @1.first_line, state->cur_status->name, $1);
            YYABORT;
        }
    }
    | TOK_IDENT '=' TOK_INTEGER {
        if (status_config_set_int(state->items, $1, $3) == -1) {
            fprintf(stderr, "%s: %d: Status \"%s\" does not have integer item \"%s\"\n",
                yabr_config.config_file, @1.first_line, state->cur_status->name, $1);
            YYABORT;
        }
    }
    ;

status_property_list:
    {

    }
    | status_property
    | status_property_list ',' status_property
    | status_property_list ','
    ;

status:
    status_block_prefix '{' status_property_list '}' {
        $$ = (state->cur_status->status_create) (state->items);
        
        if (!$$) {
            fprintf(stderr, "%s: %d: %s: Invalid status configuration\n", yabr_config.config_file, @1.first_line, state->cur_status->name);
            YYABORT;
        }

        status_config_list_clear(state->items);
        free(state->items);
    }
    ;

status_list:
    {

    }
    | status {
        if ($1)
            status_list_add_tail(state->status_list, $1);
    }
    | status_list ',' status {
        if ($3)
            status_list_add_tail(state->status_list, $3);
    }
    | status_list ','
    ;


block:
    | TOK_CENTER '{' status '}' {
        yabr_config.state.centered = $3;
    }
    | right_block_prefix status_list '}'
    ;

right_block_prefix:
    TOK_RIGHT '{' {
        state->status_list = &yabr_config.state.status_list; 
    }
    ;

status_block_prefix:
    TOK_STATUS TOK_IDENT {
        size_t items_size;
        int i;

        /* Loop over statuses */
        state->cur_status = NULL;

        for (i = 0; i < DESC_TOTAL; i++) {
            if (strcmp(yabr_config.descs[i]->name, $2) == 0) {
                state->cur_status = yabr_config.descs[i];
                break;
            }
        }

        if (!state->cur_status) {
            fprintf(stderr, "%s: %d: Unknown status \"%s\"\n", yabr_config.config_file, @2.first_line, $2);
            YYABORT;
        }

        items_size = sizeof(*state->items) * (status_config_list_count(state->cur_status->items) + 1);

        state->items = malloc(items_size);
        memcpy(state->items, state->cur_status->items, items_size);

        free($2);
    }
    ;

%%

void yyerror(struct parse_state *state, const char *str)
{
    fprintf(stderr, "Parser error: %d: %s\n", yylloc.first_line, str);
}

