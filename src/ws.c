
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include <glib/gprintf.h>
#include <i3ipc-glib/i3ipc-glib.h>

#include "bar_config.h"
#include "render.h"
#include "config.h"
#include "ws.h"

void ws_clear(struct ws *ws)
{
    free(ws->name);
    free(ws->output);
}

void ws_list_clear(struct ws_list *wslist)
{
    struct ws *ws;

    if (list_empty(&wslist->list))
        return ;

    list_foreach_take_entry(&wslist->list, ws, ws_entry) {
        ws_clear(ws);
        free(ws);
    }

    return ;
}

int ws_list_refresh(struct ws_list *wslist, struct i3_state *i3)
{
    GSList *l, *list;

    ws_list_clear(wslist);

    list = i3ipc_connection_get_workspaces(i3->conn, NULL);

    for (l = list; l; l = l->next) {
        struct ws *ws;
        i3ipcWorkspaceReply *reply = l->data;

        ws = malloc(sizeof(*ws));
        ws_init(ws);

        ws->name = strdup(reply->name);
        ws->output = strdup(reply->output);

        if (reply->visible)
            flag_set(&ws->flags, WS_VISIBLE);

        if (reply->focused)
            flag_set(&ws->flags, WS_FOCUSED);

        if (reply->urgent)
            flag_set(&ws->flags, WS_URGENT);

        list_add(&wslist->list, &ws->ws_entry);
    }

    if (list)
        g_slist_free_full(list, (GDestroyNotify) i3ipc_workspace_reply_free);

    return 0;
}

void ws_switch(struct bar_state *state, const char *ws)
{
    char cmd[128];
    char *res;

    snprintf(cmd, sizeof(cmd), "workspace %s", ws);

    res = i3ipc_connection_message(state->i3_state.conn, I3IPC_MESSAGE_TYPE_COMMAND, cmd, NULL);

    dbgprintf("cmd: %s, result: %s\n", cmd, res);

    g_free(res);
}

void ws_next(struct bar_state *state, const char *output)
{
    struct ws *ws, *first = NULL, *found = NULL;

    list_foreach_entry(&state->ws_list.list, ws, ws_entry) {
        if (strcmp(ws->output, output) == 0) {
            if (!first)
                first = ws;

            if (!found && flag_test(&ws->flags, WS_FOCUSED)) {
                found = ws;
            } else if (found) {
                ws_switch(state, ws->name);
                return ;
            }
        }
    }

    if (!found)
        ws_switch(state, first->name);

    return ;
}

void ws_prev(struct bar_state *state, const char *output)
{
    struct ws *ws, *last = NULL, *found = NULL;

    list_foreach_entry_reverse(&state->ws_list.list, ws, ws_entry) {
        if (strcmp(ws->output, output) == 0) {
            if (!last)
                last = ws;

            if (!found && flag_test(&ws->flags, WS_FOCUSED)) {
                found = ws;
            } else if (found) {
                ws_switch(state, ws->name);
                return ;
            }
        }
    }

    if (!found)
        ws_switch(state, last->name);

    return ;
}

