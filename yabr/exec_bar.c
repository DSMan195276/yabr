
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "bar_config.h"

FILE *lemonbar_open(void)
{
    char prog_string[256];
    char fore[10], back[10];
    FILE *prog;

    sprintf(fore, "#%08x", BAR_DEFAULT_COLOR_FORE);
    sprintf(back, "#%08x", BAR_DEFAULT_COLOR_BACK);

    snprintf(prog_string, sizeof(prog_string),
            "lemonbar -g " LEMONBAR_GEOMETRY " -f " LEMONBAR_FONT " -F%s -B%s | sh >/dev/null",
            fore, back);

    fprintf(stderr, "Lemonbar exec: %s\n", prog_string);

    prog = popen(prog_string, "w");

    return prog;
}

