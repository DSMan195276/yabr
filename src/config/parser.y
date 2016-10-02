%{
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "config.h"
#include "list.h"
#include "render.h"
#include "status.h"
#include "lexer.h"

#define IN_PARSER
#include "parser.h"

void yyerror(struct parse_state *state, const char *str);

list_head_t variable_list = LIST_HEAD_INIT(variable_list);

struct variable *variable_find(const char *id);
int global_assign_color_pair(const char *id, struct bar_color pair);
int global_assign_integer(const char *id, uint32_t);
int global_assign_string(const char *id, char *val);

#define YYERROR_VERBOSE
%}

%union {
    char *str;
    uint32_t ival;
    struct bar_color color_pair;
    struct status *status;

    struct {
        int pair_count;
        struct bar_color *pairs;
    } color_pair_list;

    struct {
        int type;
        union var_data data;
    } var;
}

%parse-param { struct parse_state *state }
%locations

%token <str> TOK_IDENT
%token <str> TOK_STRING
%token TOK_STATUS
%token TOK_CENTER
%token TOK_RIGHT
%token TOK_VAR
%token <ival> TOK_INTEGER
%token TOK_EOF TOK_ERR

%type <status> status
%type <ival> integer;
%type <ival> integer_or_var;
%type <color_pair> color_pair
%type <color_pair> color_pair_or_var
%type <color_pair_list> color_pair_list
%type <var> variable_type

%right '!'
%left  '|' '&'
%left  '*' '/'
%left  '+' '-'

%start config_file

%%

integer_or_var:
    integer
    | TOK_IDENT {
        struct variable *var;

        var = variable_find($1);

        if (!var) {
            fprintf(stderr, "%s: %d: No variable named \"%s\"\n",
                yabr_config.config_file, @1.first_line, $1);
            YYABORT;
        }

        if (var->type != VAR_INT) {
            fprintf(stderr, "%s: %d: Variable named \"%s\" is not a color/integer.\n",
                yabr_config.config_file, @1.first_line, $1);
            YYABORT;
        }
        
        $$ = var->data.i;
        free($1);
    }
    ;

integer:
    TOK_INTEGER
    | integer_or_var '|' integer_or_var {
        $$ = $1 | $3;
    }
    | integer_or_var '&' integer_or_var {
        $$ = $1 & $3;
    }
    | integer_or_var '*' integer_or_var {
        $$ = $1 * $3;
    }
    | integer_or_var '/' integer_or_var {
        $$ = $1 / $3;
    }
    | '!' integer_or_var {
        $$ = !$2;
    }
    | '(' integer_or_var ')' {
        $$ = $2;
    }
    ;

color_pair:
    '{' integer_or_var ',' integer_or_var '}' {
        $$.fore = $2;
        $$.back = $4;
    }
    ;

color_pair_or_var:
    color_pair
    | TOK_IDENT {
        struct variable *var;

        var = variable_find($1);

        if (!var) {
            fprintf(stderr, "%s: %d: No variable named \"%s\"\n",
                yabr_config.config_file, @1.first_line, $1);
            YYABORT;
        }

        if (var->type != VAR_COLOR_PAIR) {
            fprintf(stderr, "%s: %d: Variable named \"%s\" is not a color pair.\n",
                yabr_config.config_file, @1.first_line, $1);
            YYABORT;
        }
        
        $$ = var->data.color_pair;

        free($1);
    }
    ;


variable_type:
    TOK_STRING    { $$.type = VAR_STRING;     $$.data.str = $1; }
    | integer     { $$.type = VAR_INT;        $$.data.i = $1; }
    | color_pair  { $$.type = VAR_COLOR_PAIR; $$.data.color_pair = $1; }
    | TOK_IDENT {
        struct variable *var;

        var = variable_find($1);

        if (!var) {
            fprintf(stderr, "%s: %d: No variable named \"%s\"\n",
                yabr_config.config_file, @1.first_line, $1);
            YYABORT;
        }

        $$.type = var->type;

        switch (var->type) {
        case VAR_STRING:
            $$.data.str = strdup(var->data.str);
            break;

        case VAR_COLOR_PAIR:
        case VAR_INT:
            $$.data = var->data;
            break;
        }

        free($1);
    }

variable:
    TOK_VAR TOK_IDENT '=' variable_type ';' {
        struct variable *var = malloc(sizeof(*var));
        var->id = $2;

        var->type = $4.type;
        var->data = $4.data;

        list_add_tail(&variable_list, &var->variable_entry);
    }
    ;

status_property:
    TOK_IDENT '=' TOK_STRING ';' {
        if (status_config_set_str(state->items, $1, $3) == -1) {
            fprintf(stderr, "%s: %d: Status \"%s\" does not have string item \"%s\"\n",
                yabr_config.config_file, @1.first_line, state->cur_status->name, $1);
            YYABORT;
        }

        free($1);
        free($3);
    }
    | TOK_IDENT '=' integer ';' {
        if (status_config_set_int(state->items, $1, $3) == -1) {
            fprintf(stderr, "%s: %d: Status \"%s\" does not have integer item \"%s\"\n",
                yabr_config.config_file, @1.first_line, state->cur_status->name, $1);
            YYABORT;
        }

        free($1);
    }
    | TOK_IDENT '=' TOK_IDENT ';' {
        struct variable *var;
        struct status_config_item *item;

        item = status_config_get(state->items, $1);

        if (!item) {
            fprintf(stderr, "%s: %d: Status \"%s\" does not have item \"%s\"\n",
                yabr_config.config_file, @1.first_line, state->cur_status->name, $1);
            YYABORT;
        }

        var = variable_find($3);

        if (!var) {
            fprintf(stderr, "%s: %d: No variable named \"%s\"\n",
                yabr_config.config_file, @3.first_line, $3);
            YYABORT;
        }

        if (item->type == STATUS_CONFIG_INT && var->type == VAR_INT) {
            status_config_item_set_int(item, var->data.i);
        } else if (item->type == STATUS_CONFIG_STRING && var->type == VAR_STRING) {
            status_config_item_set_str(item, var->data.str);
        } else {
            fprintf(stderr, "%s: %d: Variable \"%s\" has wrong type for item \"%s\"\n",
                yabr_config.config_file, @3.first_line, $3, $1);
            YYABORT;
        }

        free($3);
        free($1);
    }
    ;

status_property_list: %empty
    | status_property_list status_property
    ;

right_block_prefix:
    TOK_RIGHT {
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

status_list: %empty
    | status_list status {
        if ($2)
            status_list_add_tail(state->status_list, $2);
    }
    ;

color_pair_list:
    color_pair_or_var {
        $$.pair_count = 1;
        $$.pairs = malloc(sizeof(*$$.pairs) * $$.pair_count);
        $$.pairs[0] = $1;
    }
    | color_pair_list ',' color_pair_or_var {
        $$.pair_count++;
        $$.pairs = realloc($$.pairs, sizeof(*$$.pairs) * $$.pair_count);
        $$.pairs[$$.pair_count - 1] = $3;
    }
    ;

block:
    TOK_CENTER '{' status '}' {
        yabr_config.state.centered = $3;
    }
    | right_block_prefix '{' status_list '}'
    ;

global_assignment:
    TOK_IDENT '=' TOK_STRING ';' {
        if (global_assign_string($1, $3) == -1) {
            fprintf(stderr, "%s: %d: Unknown string \"%s\"\n", yabr_config.config_file, @1.first_line, $1);
            YYABORT;
        }

        free($1);
    }
    | TOK_IDENT '=' integer ';' {
        if (global_assign_integer($1, $3) == -1) {
            fprintf(stderr, "%s: %d: Unknown integer \"%s\"\n", yabr_config.config_file, @1.first_line, $1);
            YYABORT;
        }

        free($1);
    }
    | TOK_IDENT '=' color_pair ';' {
        if (global_assign_color_pair($1, $3) == -1) {
            fprintf(stderr, "%s: %d: Unknown color pair \"%s\"\n", yabr_config.config_file, @1.first_line, $1);
            YYABORT;
        }

        free($1);
    }
    | TOK_IDENT '=' '[' color_pair_list ']' ';' {
        if (strcmp($1, "sections") == 0) {
            yabr_config.colors.section_count = $4.pair_count;
            yabr_config.colors.section_cols = $4.pairs;
        } else {
            fprintf(stderr, "%s: %d: Unknown color pair list \"%s\"\n", yabr_config.config_file, @1.first_line, $1);
            YYABORT;
        }

        free($1);
    }
    | TOK_IDENT '=' TOK_IDENT ';' {
        struct variable *var = variable_find($3);

        if (!var) {
            fprintf(stderr, "%s: %d: No variable named \"%s\"\n",
                yabr_config.config_file, @1.first_line, $1);
            YYABORT;
        }

        switch (var->type) {
        case VAR_INT:
            if (global_assign_integer($1, var->data.i) == -1) {
                fprintf(stderr, "%s: %d: Unknown integer \"%s\"\n", yabr_config.config_file, @1.first_line, $1);
                YYABORT;
            }
            break;

        case VAR_STRING:
            if (global_assign_string($1, var->data.str) == -1) {
                fprintf(stderr, "%s: %d: Unknown string \"%s\"\n", yabr_config.config_file, @1.first_line, $1);
                YYABORT;
            }
            break;

        case VAR_COLOR_PAIR:
            if (global_assign_color_pair($1, var->data.color_pair) == -1) {
                fprintf(stderr, "%s: %d: Unknown color_pair \"%s\"\n", yabr_config.config_file, @1.first_line, $1);
                YYABORT;
            }
            break;

        }

        free($3);
        free($1);
    }
    ;

config_file: %empty
    | config_file block
    | config_file global_assignment
    | config_file variable
    | config_file TOK_ERR {
        YYABORT;
    }
    | config_file TOK_EOF {
        struct variable *var;

        list_foreach_take_entry(&variable_list, var, variable_entry) {
            switch (var->type) {
            case VAR_STRING:
                free(var->data.str);
                break;

            case VAR_INT:
            case VAR_COLOR_PAIR:
                break;
            }

            free(var->id);
            free(var);
        }
        YYACCEPT;
    }
    ;

%%

void yyerror(struct parse_state *state, const char *str)
{
    fprintf(stderr, "Parser error: %d: %s\n", yylloc.first_line, str);
}

struct variable *variable_find(const char *id)
{
    struct variable *var;

    list_foreach_entry(&variable_list, var, variable_entry)
        if (strcmp(var->id, id) == 0)
            return var;

    return NULL;
}

int global_assign_color_pair(const char *id, struct bar_color pair)
{
    if (strcmp(id, "wsp") == 0) {
        yabr_config.colors.wsp = pair;
    } else if (strcmp(id, "wsp_focused") == 0) {
        yabr_config.colors.wsp_focused = pair;
    } else if (strcmp(id, "wsp_unfocused") == 0) {
        yabr_config.colors.wsp_unfocused = pair;
    } else if (strcmp(id, "wsp_urgent") == 0) {
        yabr_config.colors.wsp_urgent = pair;
    } else if (strcmp(id, "title") == 0) {
        yabr_config.colors.title = pair;
    } else if (strcmp(id, "status_last") == 0) {
        yabr_config.colors.status_last = pair;
    } else if (strcmp(id, "mode") == 0) {
        yabr_config.colors.mode = pair;
    } else if (strcmp(id, "status_urgent") == 0) {
        yabr_config.colors.status_urgent = pair;
    } else if (strcmp(id, "centered") == 0) {
        yabr_config.colors.centered = pair;
    } else if (strcmp(id, "def") == 0) {
        yabr_config.colors.def = pair;
    } else {
        return -1;
    }

    return 0;
}

int global_assign_integer(const char *id, uint32_t integer)
{
    if (strcmp(id, "use_separator") == 0) {
        yabr_config.use_separator = integer;
    } else if (strcmp(id, "lemonbar_font_size") == 0) {
        yabr_config.lemonbar_font_size = integer;
    } else if (strcmp(id, "lemonbar_on_bottom") == 0) {
        yabr_config.lemonbar_on_bottom = integer;
    } else {
        return -1;
    }

    return 0;
}

int global_assign_string(const char *id, char *str)
{
    if (strcmp(id, "sep_rightside") == 0) {
        yabr_config.sep_rightside = str;
    } else if (strcmp(id, "sep_leftside") == 0) {
        yabr_config.sep_leftside = str;
    } else if (strcmp(id, "sep_rightside_same") == 0) {
        yabr_config.sep_rightside_same = str;
    } else if (strcmp(id, "sep_leftside_same") == 0) {
        yabr_config.sep_leftside_same = str;
    } else if (strcmp(id, "lemonbar_font") == 0) {
        yabr_config.lemonbar_font = str;
    } else {
        return -1;
    }

    return 0;
}

