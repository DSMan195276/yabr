
#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "list.h"
#include "status.h"
#include "lexer.h"
#include "parser.h"

int load_config_file(void)
{
    struct parse_state state;
    FILE *file = fopen(yabr_config.config_file, "r");
    int ret;

    if (!file) {
        perror(yabr_config.config_file);
        return -1;
    }

    memset(&state, 0, sizeof(state));

    yyin = file;
    ret = yyparse(&state);

    fclose(file);
    return ret;
}

