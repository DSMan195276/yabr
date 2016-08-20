#ifndef INCLUDE_RENDER_H
#define INCLUDE_RENDER_H

#include <stdio.h>
#include <stdint.h>

#include "list.h"
#include "flag.h"
#include "ws.h"

/* A pair representing a color for the bar. */
struct bar_color {
    uint32_t fore, back;
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
};

enum status_flags {
    STATUS_VISIBLE,
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


void status_list_add(struct status_list *, struct status *);

/*
 * Holds all the colective information on the state of the bar.
 */
struct bar_state {
    struct ws_list ws_list;
    char *win_title;
    struct status_list status_list;

    struct bar_color color;
    FILE *bar_output;
    const char *output_title;

    struct bar_color prev_color;
    int sep_direction, past_first_entry;
};

void bar_state_render(struct bar_state *);

extern struct bar_state bar_state;

#endif
