#ifndef INCLUDE_CONFIG_H
#define INCLUDE_CONFIG_H

#include <stdio.h>
#include <stdint.h>
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


struct yabr_config_colors {
    struct bar_color wsp_focused;
    struct bar_color wsp_unfocused;
    struct bar_color wsp_urgent;
    struct bar_color title;

    struct bar_color status_last;
    struct bar_color mode;
    struct bar_color status_urgent;
    struct bar_color centered;
    struct bar_color background;

    int section_count;
    struct bar_color *section_cols;
};

struct yabr_config {
    FILE *debug;
    char *config_file;

    struct yabr_config_colors colors;

    int use_separator;

    char *sep_rightside, *sep_leftside;
    char *sep_rightside_same, *sep_leftside_same;

    char *lemonbar_font;
    int lemonbar_font_size;
    int lemonbar_on_bottom;

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
