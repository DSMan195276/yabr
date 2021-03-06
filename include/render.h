#ifndef INCLUDE_RENDER_H
#define INCLUDE_RENDER_H

#include <stdio.h>
#include <stdint.h>

#include <glib/gprintf.h>

#include "bar_config.h"
#include "list.h"
#include "flag.h"
#include "ws.h"
#include "i3.h"
#include "notif_dbus.h"
#include "lemonbar.h"
#include "status.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*(arr)))

/* A pair representing a color for the bar. */
struct bar_color {
    uint32_t fore, back;
};

struct bar_output {
    char *name;
    FILE *bar;

    int x, y, width, height;

    struct lemonbar lemon;
    GIOChannel *cmd_channel;
};

/*
 * Holds all the colective information on the state of the bar.
 */
struct bar_state {
    struct ws_list ws_list;
    char *win_title;
    struct status_list status_list;
    struct status *centered;

    struct bar_color color;
    int cur_status_color;
    char *mode;

    int output_count;
    struct bar_output outputs[BAR_MAX_OUTPUTS];

    struct bar_color prev_color;
    int sep_direction, past_first_entry;

    struct i3_state i3_state;

    struct notif_dbus notif;
};

#define BAR_STATE_INIT(state) \
    { \
        .ws_list = WS_LIST_INIT((state).ws_list), \
        .status_list = STATUS_LIST_INIT((state).status_list), \
    }

void bar_state_render(struct bar_state *);

void bar_render_global(void);

#endif
