
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <glib/gprintf.h>
#include <i3ipc-glib/i3ipc-glib.h>

#include "bar_config.h"
#include "render.h"

static struct bar_color status_colors[] = {
    { .fore = COLOR_FORE, .back = COLOR_SEC_B1 },
    { .fore = COLOR_FORE, .back = COLOR_SEC_B2 },
    { .fore = COLOR_FORE, .back = COLOR_SEC_B3 },
    { .fore = 0, .back = 0 }
};

static struct bar_color status_last_color = {
    .fore = BAR_COLOR_STATUS_LAST_FORE, .back = BAR_COLOR_STATUS_LAST_BACK
};

#define STATUS_COLORS_COUNT ((sizeof(status_colors)/sizeof(*status_colors)) - 1)

void status_list_add(struct status_list *status_list, struct status *status)
{
    list_add(&status_list->list, &status->status_entry);
}

void status_change_text(struct status *status, const char *text)
{
    if (status->text)
        free(status->text);

    status->text = strdup(text);
}

static void render_color_last(struct bar_state *state)
{
    fprintf(state->bar_output, "%%{F#%08x B#%08x}", state->color.fore, state->color.back);
    state->prev_color = state->color;
}

static void render_color(struct bar_state *state)
{
    if (BAR_USE_SEP && state->past_first_entry) {
        if (state->prev_color.back != state->color.back) {
            if (state->sep_direction == 0) {
                fprintf(state->bar_output, "%%{F#%08x B#%08x}", state->prev_color.back, state->color.back);
                fprintf(state->bar_output, "%s", BAR_SEP_LEFTSIDE);
            } else {
                fprintf(state->bar_output, "%%{F#%08x B#%08x}", state->color.back, state->prev_color.back);
                fprintf(state->bar_output, "%s", BAR_SEP_RIGHTSIDE);
            }
        } else {
            if (state->sep_direction == 0) {
                fprintf(state->bar_output, "%%{F#%08x B#%08x}", state->color.fore, state->color.back);
                fprintf(state->bar_output, "%s", BAR_SEP_LEFTSIDE_SAME);
            } else {
                fprintf(state->bar_output, "%%{F#%08x B#%08x}", state->color.fore, state->color.back);
                fprintf(state->bar_output, "%s", BAR_SEP_RIGHTSIDE_SAME);
            }
        }
    }
    state->past_first_entry = 1;
    render_color_last(state);
}

static void render_cmd(struct bar_state *state, int button, const char *cmd)
{
    fprintf(state->bar_output, "%%{A%d:%s:}", button, cmd);
}

static void render_cmd_end(struct bar_state *state)
{
    fprintf(state->bar_output, "%%{A}");
}

static void render_right_align(struct bar_state *state)
{
    fprintf(state->bar_output, "%%{r}");
    state->sep_direction = 1;
}

static void render_left_align(struct bar_state *state)
{
    fprintf(state->bar_output, "%%{l}");
}

static void render_finish(struct bar_state *state)
{
    fputc('\n', state->bar_output);
    fflush(state->bar_output);

    state->sep_direction = 0;
    state->past_first_entry = 0;
}


static void status_render(struct bar_state *state)
{
    int cur_status_color = 0;
    struct status *status, *last_entry = list_last_entry(&state->status_list.list, struct status, status_entry);

    list_foreach_entry(&state->status_list.list, status, status_entry) {
        int cmd_count = 0, i;
        if (!flag_test(&status->flags, STATUS_VISIBLE))
            continue;

        if (!status->text)
            continue;

        for (i = 0; i < ARRAY_SIZE(status->cmds); i++) {
            if (status->cmds[i].cmd) {
                cmd_count++;
                render_cmd(state, i + 1, status->cmds[i].cmd);
            }
        }

        if (flag_test(&status->flags, STATUS_URGENT)) {
            state->color.fore = BAR_COLOR_STATUS_URGENT_FORE;
            state->color.back = BAR_COLOR_STATUS_URGENT_BACK;
        } else {
            if (status == last_entry)
                state->color = status_last_color;
            else
                state->color = status_colors[cur_status_color];

            cur_status_color = (cur_status_color + 1) % STATUS_COLORS_COUNT;
        }

        render_color(state);
        fprintf(state->bar_output, " %s ", status->text);

        for (i = 0; i < cmd_count; i++)
            render_cmd_end(state);
    }
}

static void mode_render(struct bar_state *state)
{
    struct bar_color mode_color = {
        .fore = BAR_COLOR_MODE_FORE,
        .back = BAR_COLOR_MODE_BACK,
    };

    if (!state->mode)
        return ;

    state->color = mode_color;
    render_color(state);

    fprintf(state->bar_output, " %s ", state->mode);
}

static void title_render(struct bar_state *state)
{
    state->color.fore = BAR_COLOR_TITLE_FORE;
    state->color.back = BAR_COLOR_TITLE_BACK;
    render_color(state);
    fprintf(state->bar_output, " %s ", state->win_title);
}

static void ws_list_render(struct bar_state *state)
{
    char cmdbuf[128];

    render_cmd(state, 4, "i3-msg workspace prev");
    render_cmd(state, 5, "i3-msg workspace next");

    struct ws *ws;
    list_foreach_entry(&state->ws_list.list, ws, ws_entry) {
        if (strcmp(ws->output, state->output_title) != 0)
            continue;

        if (flag_test(&ws->flags, WS_URGENT)) {
            state->color.fore = BAR_COLOR_WSP_URGENT_FORE;
            state->color.back = BAR_COLOR_WSP_URGENT_BACK;
        } else if (flag_test(&ws->flags, WS_FOCUSED)) {
            state->color.fore = BAR_COLOR_WSP_FOCUSED_FORE;
            state->color.back = BAR_COLOR_WSP_FOCUSED_BACK;
        } else {
            state->color.fore = BAR_COLOR_WSP_UNFOCUSED_FORE;
            state->color.back = BAR_COLOR_WSP_UNFOCUSED_BACK;
        }

        snprintf(cmdbuf, sizeof(cmdbuf), "i3-msg workspace %s", ws->name);

        render_cmd(state, 1, cmdbuf);
        render_color(state);

        fprintf(state->bar_output, " %s ", ws->name);

        render_cmd_end(state);
    }

    render_cmd_end(state);
    render_cmd_end(state);
}

void bar_state_render(struct bar_state *state)
{
    struct bar_color def_color = {
        .fore = COLOR_FORE,
        .back = COLOR_BACK
    };

    render_left_align(state);
    ws_list_render(state);
    mode_render(state);
    title_render(state);

    state->color = def_color;
    render_color(state);

    render_right_align(state);
    status_render(state);

    state->color = def_color;

    render_color_last(state);
    render_finish(state);
}

