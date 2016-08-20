
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include <glib/gprintf.h>
#include <i3ipc-glib/i3ipc-glib.h>

#include "bar_config.h"
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

int ws_list_refresh(struct ws_list *wslist, i3ipcConnection *conn)
{
    GSList *l, *list;

    ws_list_clear(wslist);

    list = i3ipc_connection_get_workspaces(conn, NULL);

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

