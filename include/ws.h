#ifndef INCLUDE_WC_H
#define INCLUDE_WC_H

#include <i3ipc-glib/i3ipc-glib.h>

#include <stdint.h>

#include "list.h"
#include "flag.h"

struct bar_state;

/* Describes a workspace */
struct ws {
    char *name;
    char *output;
    uint32_t flags;

    list_node_t ws_entry;
};

enum ws_flag {
    WS_VISIBLE,
    WS_FOCUSED,
    WS_URGENT,
};

#define WS_INIT(ws) \
    { \
        .ws_entry = LIST_NODE_INIT((ws).ws_entry), \
    }

static inline void ws_init(struct ws *ws)
{
    *ws = (struct ws)WS_INIT(*ws);
}

/* Holds a list of workspaces */
struct ws_list {
    list_head_t list;
};

#define WS_LIST_INIT(ws) \
    { \
        .list = LIST_HEAD_INIT((ws).list), \
    }

static inline void ws_list_init(struct ws_list *ws_list)
{
    *ws_list = (struct ws_list)WS_LIST_INIT(*ws_list);
}

void ws_clear(struct ws *);
void ws_list_clear(struct ws_list *);

int ws_list_refresh(struct ws_list *, i3ipcConnection *);

void ws_next(struct bar_state *, const char *output);
void ws_prev(struct bar_state *, const char *output);
void ws_switch(struct bar_state *, const char *ws);

#endif
