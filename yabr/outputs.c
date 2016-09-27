
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

static void lemonbar_start(struct bar_output *output)
{
    char prog_string[256];
    char geometry[20];
    char fore[10], back[10];

    sprintf(fore, "#%08x", BAR_DEFAULT_COLOR_FORE);
    sprintf(back, "#%08x", BAR_DEFAULT_COLOR_BACK);

    sprintf(geometry, "%dx14+%d+0", output->width, output->x);

    snprintf(prog_string, sizeof(prog_string),
            "lemonbar -a 20 -g %s -f " LEMONBAR_FONT " -F%s -B%s 2>./bar_stderr.txt | sh >/dev/null",
            geometry, fore, back);

    fprintf(stderr, "Output: %s, lemonbar: %s\n", output->name, prog_string);

    output->bar = popen(prog_string, "w");

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

        fprintf(stderr, "output: %s, active: %d\n", reply->name, reply->active);

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

    fprintf(stderr, "count: %d, list: %p\n", count, list);

    if (count != state->output_count || list)
        return 1;

    return 0;
}

static void outputs_clear(struct bar_state *state)
{
    int i;

    for (i = 0; i < state->output_count; i++) {
        pclose(state->outputs[i].bar);
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
            fprintf(stderr, "Error! Too many outputs, max: %zd\n", ARRAY_SIZE(state->outputs));
            exit(EXIT_FAILURE);
        }

        output->x = reply->rect->x;
        output->y = reply->rect->y;
        output->width = reply->rect->width;
        output->height = reply->rect->height;

        output->name = strdup(reply->name);

        fprintf(stderr, "Adding output: %s\n", output->name);
        fprintf(stderr, "At: %dx%d+%d+%d\n", output->width, output->height, output->x, output->y);

        lemonbar_start(output);

        fprintf(stderr, "lemonbar running on %s\n", output->name);

        count++;
    }

    state->output_count = count;
}

void outputs_update(i3ipcConnection *conn, struct bar_state *state)
{
    GSList *outputs = i3ipc_connection_get_outputs(conn, NULL);

    fprintf(stderr, "Outputs update..\n");

    if (!outputs)
        return ;

    fprintf(stderr, "Outputs valid.\n");
    if (outputs_compare(outputs, state) == 0)
        goto cleanup_outputs;

    fprintf(stderr, "Outputs different.\n");

    outputs_clear(state);
    outputs_load(state, outputs);

    /* bar_state_render(state) */

  cleanup_outputs:
    if (outputs)
        g_slist_free_full(outputs, (GDestroyNotify) i3ipc_output_reply_free);

    return ;
}

