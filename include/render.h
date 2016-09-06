#ifndef INCLUDE_RENDER_H
#define INCLUDE_RENDER_H

#include <stdio.h>
#include <stdint.h>

#include "bar_config.h"
#include "list.h"
#include "flag.h"
#include "ws.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*(arr)))

/* A pair representing a color for the bar. */
struct bar_color {
    uint32_t fore, back;
};

struct cmd_entry {
    char *cmd;
};

/* 
 * A 'status' represents a single entry into the right-side of the bar.
 *
 * These are added to the list on start-up. If the STATUS_VISIBLE flag is set,
 * then the status will be rendered onto the screen (Thus, status's can be
 * entered but only rendered when it makes sense to do so).
 */
struct status {
    char *text;
    uint32_t flags;

    list_node_t status_entry;

    struct cmd_entry cmds[5];
};

enum status_flags {
    STATUS_VISIBLE,
    STATUS_URGENT,
};

#define STATUS_INIT(status) \
    { \
        .status_entry = LIST_NODE_INIT((status).status_entry), \
    }

static inline void status_init(struct status *status)
{
    *status = (struct status)STATUS_INIT(*status);
}

struct status_list {
    list_head_t list;
};

#define STATUS_LIST_INIT(status) \
    { \
        .list = LIST_HEAD_INIT((status).list), \
    }

static inline void status_list_init(struct status_list *status)
{
    *status = (struct status_list)STATUS_LIST_INIT(*status);
}

void status_change_text(struct status *, const char *text);
void status_list_add(struct status_list *, struct status *);

struct bar_output {
    FILE *bar;
    char *name;

    int x, y, width, height;
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
};

void bar_state_render(struct bar_state *);

void bar_render_global(void);

#endif
