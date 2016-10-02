#ifndef YABR_CONFIG_PARSER_H
#define YABR_CONFIG_PARSER_H

#include "status.h"
#include "status_desc.h"

struct parse_state {
    const struct status_description *cur_status;

    struct status_config_item *items;

    struct status_list *status_list;
};

int yyparse(struct parse_state *state);

enum var_type {
    VAR_STRING,
    VAR_INT,
    VAR_COLOR_PAIR,
};

union var_data {
    char *str;
    uint32_t i;
    struct bar_color color_pair;
};

struct variable {
    list_node_t variable_entry;

    char *id;
    enum var_type type;
    union var_data data;
};


#ifndef IN_PARSER
# include "parser.tab.h"
#endif

#endif
