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

#ifndef IN_PARSER
# include "parser.tab.h"
#endif

#endif
