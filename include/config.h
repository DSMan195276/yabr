#ifndef INCLUDE_CONFIG_H
#define INCLUDE_CONFIG_H

#include <stdio.h>
#include "list.h"
#include "render.h"
#include "status_desc.h"

enum {
    DESC_ALSA,
    DESC_BATTERY,
    DESC_DATETIME,
    DESC_MAIL,
    DESC_MPD,
    DESC_TASKS,
    DESC_WIRELESS,
    DESC_TOTAL,
};

struct yabr_config {
    FILE *debug;
    char *config_file;

    struct bar_state state;

    const struct status_description *descs[DESC_TOTAL];
};

extern struct yabr_config yabr_config;

#define dbgprintf(...) \
    do { \
        if (yabr_config.debug) { \
            fprintf(yabr_config.debug, __VA_ARGS__); \
            fflush(yabr_config.debug); \
        } \
    } while (0)

#endif
