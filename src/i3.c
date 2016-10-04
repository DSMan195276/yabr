
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <i3ipc-glib/i3ipc-glib.h>

#include "render.h"
#include "outputs.h"
#include "i3.h"

static void set_window_title(struct bar_state *state, const char *title)
{
    if (&state->win_title)
        free(state->win_title);

    if (title)
        state->win_title = strdup(title);
    else
        state->win_title = strdup("");
}

void i3_state_update_title(struct bar_state *state)
{
    i3ipcCon *con, *focused;

    con = i3ipc_connection_get_tree(state->i3_state.conn, NULL);

    focused = i3ipc_con_find_focused(con);
    if (focused)
        set_window_title(state, i3ipc_con_get_name(focused));
    else
        set_window_title(state, "");

    g_object_unref(con);

}

static void workspace_change(i3ipcConnection *conn, i3ipcWorkspaceEvent *e, gpointer data)
{
    struct bar_state *state = data;
    i3ipcCon *con;

    ws_list_refresh(&state->ws_list, &state->i3_state);

    con = i3ipc_con_find_focused(e->current);
    if (con)
        set_window_title(state, i3ipc_con_get_name(con));
    else
        set_window_title(state, "");

    bar_state_render(state);
}

static void window_change(i3ipcConnection *conn, i3ipcWindowEvent *e, gpointer data)
{
    struct bar_state *state = data;

    if (e->container)
        set_window_title(state, i3ipc_con_get_name(e->container));
    else
        set_window_title(state, "");

    bar_state_render(state);
}

static void window_close(i3ipcConnection *conn, i3ipcWindowEvent *e, gpointer data)
{
    struct bar_state *state = data;
    gboolean focused;
    g_object_get(e->container, "focused", &focused, NULL);

    if (focused)
        set_window_title(state, "");

    bar_state_render(state);
}

static void mode_change(i3ipcConnection *conn, i3ipcGenericEvent *e, gpointer data)
{
    struct bar_state *state = data;
    if (state->mode)
        free(state->mode);

    if (strcmp(e->change, "default") == 0)
        state->mode = NULL;
    else
        state->mode = strdup(e->change);

    bar_state_render(state);
}

static void output_change(i3ipcConnection *conn, i3ipcGenericEvent *e, gpointer data)
{
    struct bar_state *state = data;
    outputs_update(&state->i3_state, state);

    bar_state_render(state);
}

int i3_state_setup(struct bar_state *state)
{
    state->i3_state.conn = i3ipc_connection_new(NULL, NULL);

    i3ipc_connection_on(state->i3_state.conn, "workspace::focus", g_cclosure_new(G_CALLBACK(workspace_change), state, NULL), NULL);
    i3ipc_connection_on(state->i3_state.conn, "workspace::init", g_cclosure_new(G_CALLBACK(workspace_change), state, NULL), NULL);
    i3ipc_connection_on(state->i3_state.conn, "workspace::urgent", g_cclosure_new(G_CALLBACK(workspace_change), state, NULL), NULL);
    i3ipc_connection_on(state->i3_state.conn, "workspace::empty", g_cclosure_new(G_CALLBACK(workspace_change), state, NULL), NULL);

    i3ipc_connection_on(state->i3_state.conn, "window::title", g_cclosure_new(G_CALLBACK(window_change), state, NULL), NULL);
    i3ipc_connection_on(state->i3_state.conn, "window::focus", g_cclosure_new(G_CALLBACK(window_change), state, NULL), NULL);

    i3ipc_connection_on(state->i3_state.conn, "window::close", g_cclosure_new(G_CALLBACK(window_close), state, NULL), NULL);

    i3ipc_connection_on(state->i3_state.conn, "mode", g_cclosure_new(G_CALLBACK(mode_change), state, NULL), NULL);

    i3ipc_connection_on(state->i3_state.conn, "output", g_cclosure_new(G_CALLBACK(output_change), state, NULL), NULL);

    return 0;
}

void i3_state_main(struct bar_state *state)
{
    i3ipc_connection_main(state->i3_state.conn);
}

void i3_state_close(struct bar_state *state)
{
    g_object_unref(state->i3_state.conn);
}

