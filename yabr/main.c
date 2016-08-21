
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include <glib/gprintf.h>
#include <i3ipc-glib/i3ipc-glib.h>

#include "bar_config.h"
#include "ws.h"
#include "render.h"
#include "lemonbar.h"
#include "status.h"

struct bar_state bar_state = {
    .ws_list = WS_LIST_INIT(bar_state.ws_list),
    .win_title = NULL,
    .status_list = STATUS_LIST_INIT(bar_state.status_list),
};

static void set_window_title(const char *title)
{
    if (bar_state.win_title)
        free(bar_state.win_title);

    if (title)
        bar_state.win_title = strdup(title);
    else
        bar_state.win_title = strdup("");
}

static void set_initial_title(i3ipcConnection *conn)
{
    i3ipcCon *con, *focused;

    con = i3ipc_connection_get_tree(conn, NULL);

    focused = i3ipc_con_find_focused(con);
    if (focused)
        set_window_title(i3ipc_con_get_name(focused));
    else
        set_window_title("");

    g_object_unref(con);
}

static void workspace_change(i3ipcConnection *conn, i3ipcWorkspaceEvent *e, gpointer data)
{
    i3ipcCon *con;

    ws_list_refresh(&bar_state.ws_list, conn);

    con = i3ipc_con_find_focused(e->current);
    if (con)
        set_window_title(i3ipc_con_get_name(con));
    else
        set_window_title("");

    bar_state_render(&bar_state);
}

static void window_change(i3ipcConnection *conn, i3ipcWindowEvent *e, gpointer data)
{
    if (e->container)
        set_window_title(i3ipc_con_get_name(e->container));
    else
        set_window_title("");

    bar_state_render(&bar_state);
}

static void mode_change(i3ipcConnection *conn, i3ipcGenericEvent *e, gpointer data)
{
    if (bar_state.mode)
        free(bar_state.mode);

    if (strcmp(e->change, "default") == 0)
        bar_state.mode = NULL;
    else
        bar_state.mode = strdup(e->change);

    bar_state_render(&bar_state);
}

static i3ipcConnection *i3_mon_setup(void)
{
    i3ipcConnection *conn;

    conn = i3ipc_connection_new(NULL, NULL);

    i3ipc_connection_on(conn, "workspace::focus", g_cclosure_new(G_CALLBACK(workspace_change), NULL, NULL), NULL);
    i3ipc_connection_on(conn, "workspace::empty", g_cclosure_new(G_CALLBACK(workspace_change), NULL, NULL), NULL);

    i3ipc_connection_on(conn, "window::title", g_cclosure_new(G_CALLBACK(window_change), NULL, NULL), NULL);
    i3ipc_connection_on(conn, "window::focus", g_cclosure_new(G_CALLBACK(window_change), NULL, NULL), NULL);

    i3ipc_connection_on(conn, "mode", g_cclosure_new(G_CALLBACK(mode_change), NULL, NULL), NULL);

    return conn;
}

static void (*status_init_funcs[]) (i3ipcConnection *) = {
    NULL
};

int main(int argc, char **argv)
{
    i3ipcConnection *conn;
    void (**setup_func) (i3ipcConnection *);

    bar_state.output_title = "LVDS1";
    bar_state.bar_output = lemonbar_open();

    conn = i3_mon_setup();

    datetime_status_add(&bar_state, DATE_FORMAT, TIME_FORMAT, DATE_TIMEOUT);
    battery_status_add(&bar_state, BATTERY_USE, 10000);
    alsa_status_add(&bar_state, ALSA_MIX, ALSA_CARD);
    wireless_status_add(&bar_state, WIRELESS_IFACE);
    mpdmon_status_add(&bar_state, MPD_SERVER, MPD_PORT, MPD_TIMEOUT);

    for (setup_func = status_init_funcs; *setup_func; setup_func++)
        (*setup_func) (conn);

    ws_list_refresh(&bar_state.ws_list, conn);
    set_initial_title(conn);
    bar_state_render(&bar_state);

    i3ipc_connection_main(conn);

    g_object_unref(conn);

    fclose(bar_state.bar_output);
    ws_list_clear(&bar_state.ws_list);

    return 0;
}

