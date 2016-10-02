
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <inttypes.h>

#include <glib/gprintf.h>
#include <i3ipc-glib/i3ipc-glib.h>

#include "bar_config.h"
#include "ws.h"
#include "render.h"
#include "config.h"
#include "lemonbar.h"
#include "status.h"

static void run_cmd(struct status *status, const char *cmd, const char *arg)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(status->cmds); i++) {
        if (status->cmds[i].id && strcmp(status->cmds[i].id, cmd) == 0) {
            (status->cmds[i].cmd_handle) (status, status->cmds + i, arg);
            break;
        }
    }

    return ;
}

static gboolean lemonbar_handle_cmd(GIOChannel *gio, GIOCondition condition, gpointer data)
{
    struct bar_output *bar = data;
    struct status *status;
    char cmd[256];
    void *key;
    char *s_cmd, *s_arg;
    size_t len;

    fgets(cmd, sizeof(cmd), bar->lemon.read_file);

    /* Strip newline character */
    len = strlen(cmd);
    if (len)
        cmd[len - 1] = '\0';

    sscanf(cmd, "%p", &key);

    s_cmd = strchr(cmd, '-');
    if (!s_cmd)
        return TRUE;

    s_cmd++;

    s_arg = strchr(s_cmd, '-');
    if (s_arg) {
        /* If we have arguments, cut-off the arguments into a separate
         * string from the command */
        *s_arg = '\0';
        s_arg++;
    }


    if (key == &yabr_config.state) {
        if (strcmp(s_cmd, "prev") == 0) {
            ws_prev(&yabr_config.state, bar->name);
        } else if (strcmp(s_cmd, "next") == 0) {
            ws_next(&yabr_config.state, bar->name);
        } else if (strcmp(s_cmd, "switch") == 0) {
            if (s_arg)
                ws_switch(&yabr_config.state, s_arg);
        } else {
            dbgprintf("Received unknown workspace cmd: %s\n", s_cmd);
        }

        return TRUE;
    }

    if (key == yabr_config.state.centered) {
        run_cmd(yabr_config.state.centered, s_cmd, s_arg);
        return TRUE;
    }

    list_foreach_entry(&yabr_config.state.status_list.list, status, status_entry) {
        if (key == status) {
            run_cmd(status, s_cmd, s_arg);
            return TRUE;
        }
    }

    return TRUE;
}

static void lemonbar_exec(struct bar_output *output)
{
    char geometry[20];
    char fore[10], back[10];

    snprintf(fore, sizeof(fore), "#%08x", yabr_config.colors.def.fore);
    snprintf(back, sizeof(back), "#%08x", yabr_config.colors.def.back);
    snprintf(geometry, sizeof(geometry),  "%dx%d+%d+0", output->width, yabr_config.lemonbar_font_size, output->x);

    char *const argv[] = {
        "lemonbar",
        "-a", "20",
        "-g", geometry,
        "-f", yabr_config.lemonbar_font,
        "-F", fore,
        "-B", back,
        (yabr_config.lemonbar_on_bottom)? "-b": NULL,
        NULL
    };

    lemonbar_start(&output->lemon, "lemonbar", argv);

    output->cmd_channel = g_io_channel_unix_new(output->lemon.readfd[0]);
    g_io_add_watch(output->cmd_channel, G_IO_IN, lemonbar_handle_cmd, output);

    output->bar = output->lemon.write_file;

    return ;
}

/* 
 * Compare a new list of outputs to our current saved outputs 
 *
 * Returns 0 if they are the same.
 */
static int outputs_compare(GSList *list, struct bar_state *state)
{
    int count = 0;

    for (; list; list = list->next) {
        i3ipcOutputReply *reply = list->data;

        dbgprintf("output: %s, active: %d\n", reply->name, reply->active);

        if (!reply->active)
            continue;

        count++;

        int i;
        for (i = 0; i < state->output_count; i++) {
            struct bar_output *output = state->outputs + i;

            if (output->x != reply->rect->x || output->y != reply->rect->y
                || output->width != reply->rect->width || output->height != reply->rect->height)
                continue;

            if (strcmp(state->outputs[i].name, reply->name) != 0)
                continue;

            break;
        }

        if (i == state->output_count)
            return 1;
    }

    dbgprintf("count: %d, list: %p\n", count, list);

    if (count != state->output_count || list)
        return 1;

    return 0;
}

static void outputs_clear(struct bar_state *state)
{
    int i;

    for (i = 0; i < state->output_count; i++) {
        lemonbar_kill(&state->outputs[i].lemon);
        free(state->outputs[i].name);
    }

    memset(state->outputs, 0, sizeof(state->outputs));

    return ;
}

static void outputs_load(struct bar_state *state, GSList *outputs)
{
    int count = 0;

    for (; outputs; outputs = outputs->next) {
        i3ipcOutputReply *reply = outputs->data;
        struct bar_output *output = state->outputs + count;

        if (!reply->active)
            continue;

        if (count == ARRAY_SIZE(state->outputs)) {
            dbgprintf("Error! Too many outputs, max: %zd\n", ARRAY_SIZE(state->outputs));
            exit(EXIT_FAILURE);
        }

        output->x = reply->rect->x;
        output->y = reply->rect->y;
        output->width = reply->rect->width;
        output->height = reply->rect->height;

        output->name = strdup(reply->name);

        dbgprintf("Adding output: %s\n", output->name);
        dbgprintf("At: %dx%d+%d+%d\n", output->width, output->height, output->x, output->y);

        lemonbar_exec(output);

        dbgprintf("lemonbar running on %s\n", output->name);

        count++;
    }

    state->output_count = count;
}

void outputs_update(i3ipcConnection *conn, struct bar_state *state)
{
    GSList *outputs = i3ipc_connection_get_outputs(conn, NULL);

    dbgprintf("Outputs update..\n");

    if (!outputs)
        return ;

    dbgprintf("Outputs valid.\n");
    if (outputs_compare(outputs, state) == 0)
        goto cleanup_outputs;

    dbgprintf("Outputs different.\n");

    outputs_clear(state);
    outputs_load(state, outputs);

    /* bar_state_render(state) */

  cleanup_outputs:
    if (outputs)
        g_slist_free_full(outputs, (GDestroyNotify) i3ipc_output_reply_free);

    return ;
}

