
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include <glib/gprintf.h>
#include <i3ipc-glib/i3ipc-glib.h>

#include "bar_config.h"
#include "ws.h"
#include "render.h"
#include "outputs.h"
#include "status.h"
#include "config.h"
#include "yabr_parser.h"

void bar_render_global(void)
{
    bar_state_render(&yabr_config.state);
}

static void set_window_title(const char *title)
{
    if (&yabr_config.state.win_title)
        free(yabr_config.state.win_title);

    if (title)
        yabr_config.state.win_title = strdup(title);
    else
        yabr_config.state.win_title = strdup("");
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

    ws_list_refresh(&yabr_config.state.ws_list, conn);

    con = i3ipc_con_find_focused(e->current);
    if (con)
        set_window_title(i3ipc_con_get_name(con));
    else
        set_window_title("");

    bar_state_render(&yabr_config.state);
}

static void window_change(i3ipcConnection *conn, i3ipcWindowEvent *e, gpointer data)
{
    if (e->container)
        set_window_title(i3ipc_con_get_name(e->container));
    else
        set_window_title("");

    bar_state_render(&yabr_config.state);
}

static void mode_change(i3ipcConnection *conn, i3ipcGenericEvent *e, gpointer data)
{
    if (yabr_config.state.mode)
        free(yabr_config.state.mode);

    if (strcmp(e->change, "default") == 0)
        yabr_config.state.mode = NULL;
    else
        yabr_config.state.mode = strdup(e->change);

    bar_state_render(&yabr_config.state);
}

static void output_change(i3ipcConnection *conn, i3ipcGenericEvent *e, gpointer data)
{
    outputs_update(conn, &yabr_config.state);

    bar_state_render(&yabr_config.state);
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

    i3ipc_connection_on(conn, "output", g_cclosure_new(G_CALLBACK(output_change), NULL, NULL), NULL);

    return conn;
}

static void usage(const char *prog)
{
    printf("%s: [-vh] [-c config-file] [-l log-file]\n", prog);
    printf("\n");
    printf(" -v : Show version information\n");
    printf(" -h : Show this help\n");
    printf(" -c : Use 'config-file' instead of default ~/.yabrrc\n");
    printf(" -l : Log to 'log-file' instead of stderr\n");
}

static void version(const char *prog)
{
    printf("%s: Yet Another Bar Replacement\n", prog);
    printf("  Written by Matthew Kilgore\n");
}

int main(int argc, char **argv)
{
    char cfg_file[256];
    i3ipcConnection *conn;
    int ret;


    if (getenv("HOME"))
        snprintf(cfg_file, sizeof(cfg_file), "%s/.yabrrc", getenv("HOME"));
    else
        snprintf(cfg_file, sizeof(cfg_file), "/etc/yabrrc");

    yabr_config.config_file = cfg_file;

    while ((ret = getopt(argc, argv, "hvc:l:")) != -1) {
        switch (ret) {
        case 'c':
            yabr_config.config_file = optarg;
            break;

        case 'h':
            usage(argv[0]);
            exit(0);

        case 'v':
            version(argv[0]);
            exit(0);

        case 'l':
            if (yabr_config.debug) {
                fprintf(stderr, "%s: Please only supply one log file.\n", argv[0]);
                exit(1);
            }
            yabr_config.debug = fopen(optarg, "a");
            if (!yabr_config.debug) {
                perror(optarg);
                exit(1);
            }
            break;

        case '?':
            usage(argv[0]);
            exit(1);
        }
    }

    if (!yabr_config.debug)
        yabr_config.debug = stderr;

    if (optind < argc) {
        fprintf(stderr, "%s: Unused argument \"%s\"\n", argv[0], argv[optind]);
        exit(1);
    }

    ret = load_config_file();

    if (ret)
        return 1;

    conn = i3_mon_setup();

    outputs_update(conn, &yabr_config.state);

    ws_list_refresh(&yabr_config.state.ws_list, conn);
    set_initial_title(conn);
    bar_state_render(&yabr_config.state);

    i3ipc_connection_main(conn);

    g_object_unref(conn);

    ws_list_clear(&yabr_config.state.ws_list);

    return 0;
}

