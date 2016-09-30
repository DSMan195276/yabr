#ifndef INCLUDE_STATUS_H
#define INCLUDE_STATUS_H

#include <i3ipc-glib/i3ipc-glib.h>
#include <stdint.h>
#include "list.h"

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
void status_list_add_tail(struct status_list *, struct status *);

#endif
