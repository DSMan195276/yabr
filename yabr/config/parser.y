%{
#include <stdlib.h>
#include <stdio.h>

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

#define YYERROR_VERBOSE
%}

%union {
    char *str;
    int ival;
    uint32_t color;
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
%token <color> TOK_COLOR
%token TOK_STATUS
%token TOK_CENTER
%token TOK_RIGHT
%token TOK_VAR
%token <ival> TOK_INTEGER
%token TOK_EOF TOK_ERR

%type <status> status
%type <color> color
%type <color> color_or_var
%type <color_pair> color_pair
%type <color_pair> color_pair_or_var
%type <color_pair_list> color_pair_list
%type <var> variable_type

%start config_file

%%

color:
     TOK_COLOR {
        $$ = $1;
     }
     ;

color_or_var:
     color
     | TOK_IDENT {
        struct variable *var;

        var = variable_find($1);

        if (!var) {
            fprintf(stderr, "%s: %d: No variable named \"%s\"\n",
                yabr_config.config_file, @1.first_line, $1);
            YYABORT;
        }

        if (var->type != VAR_COLOR) {
            fprintf(stderr, "%s: %d: Variable named \"%s\" is not a color.\n",
                yabr_config.config_file, @1.first_line, $1);
            YYABORT;
        }
        
        $$ = var->data.color;
        free($1);
     }
     ;

color_pair:
    '{' color_or_var ',' color_or_var '}' {
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
    | TOK_INTEGER { $$.type = VAR_INT;        $$.data.i = $1; }
    | color       { $$.type = VAR_COLOR;      $$.data.color = $1; }
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

        case VAR_COLOR:
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
    | TOK_IDENT '=' TOK_INTEGER ';' {
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
        if (strcmp($1, "sep_rightside") == 0) {
            yabr_config.sep_rightside = $3;
        } else if (strcmp($1, "sep_leftside") == 0) {
            yabr_config.sep_leftside = $3;
        } else if (strcmp($1, "sep_rightside_same") == 0) {
            yabr_config.sep_rightside_same = $3;
        } else if (strcmp($1, "sep_leftside_same") == 0) {
            yabr_config.sep_leftside_same = $3;
        } else if (strcmp($1, "lemonbar_font") == 0) {
            yabr_config.lemonbar_font = $3;
        } else {
            fprintf(stderr, "%s: %d: Unknown string \"%s\"\n", yabr_config.config_file, @1.first_line, $1);
            YYABORT;
        }

        free($1);
    }
    | TOK_IDENT '=' TOK_INTEGER ';' {
        if (strcmp($1, "use_separator") == 0) {
            yabr_config.use_separator = $3;
        } else {
            fprintf(stderr, "%s: %d: Unknown integer \"%s\"\n", yabr_config.config_file, @1.first_line, $1);
            YYABORT;
        }

        free($1);
    }
    | TOK_IDENT '=' color_pair_or_var ';' {
        if (strcmp($1, "wsp") == 0) {
            yabr_config.colors.wsp = $3;
        } else if (strcmp($1, "wsp_focused") == 0) {
            yabr_config.colors.wsp_focused = $3;
        } else if (strcmp($1, "wsp_unfocused") == 0) {
            yabr_config.colors.wsp_unfocused = $3;
        } else if (strcmp($1, "wsp_urgent") == 0) {
            yabr_config.colors.wsp_urgent = $3;
        } else if (strcmp($1, "title") == 0) {
            yabr_config.colors.title = $3;
        } else if (strcmp($1, "status_last") == 0) {
            yabr_config.colors.status_last = $3;
        } else if (strcmp($1, "mode") == 0) {
            yabr_config.colors.mode = $3;
        } else if (strcmp($1, "status_urgent") == 0) {
            yabr_config.colors.status_urgent = $3;
        } else if (strcmp($1, "centered") == 0) {
            yabr_config.colors.centered = $3;
        } else if (strcmp($1, "def") == 0) {
            yabr_config.colors.def = $3;
        } else {
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
            case VAR_COLOR:
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
