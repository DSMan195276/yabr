
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>

#include <glib/gprintf.h>
#include <i3ipc-glib/i3ipc-glib.h>

#include "bar_config.h"
#include "config.h"
#include "render.h"

void status_list_add(struct status_list *status_list, struct status *status)
{
    list_add(&status_list->list, &status->status_entry);
}

void status_list_add_tail(struct status_list *status_list, struct status *status)
{
    list_add_tail(&status_list->list, &status->status_entry);
}

void status_change_text(struct status *status, const char *text)
{
    if (status->text)
        free(status->text);

    status->text = strdup(text);
}

static void render_color_no_sep(struct bar_state *state, struct bar_output *output)
{
    fprintf(output->bar, "%%{F#%08x B#%08x}", state->color.fore, state->color.back);
    state->prev_color = state->color;
}

static void render_color(struct bar_state *state, struct bar_output *output)
{
    if (yabr_config.use_separator && state->past_first_entry) {
        if (state->prev_color.back != state->color.back) {
            if (state->sep_direction == 0) {
                fprintf(output->bar, "%%{F#%08x B#%08x}", state->prev_color.back, state->color.back);
                fprintf(output->bar, "%s", yabr_config.sep_leftside);
            } else {
                fprintf(output->bar, "%%{F#%08x B#%08x}", state->color.back, state->prev_color.back);
                fprintf(output->bar, "%s", yabr_config.sep_rightside);
            }
        } else {
            if (state->sep_direction == 0) {
                fprintf(output->bar, "%%{F#%08x B#%08x}", state->color.fore, state->color.back);
                fprintf(output->bar, "%s", yabr_config.sep_leftside_same);
            } else {
                fprintf(output->bar, "%%{F#%08x B#%08x}", state->color.fore, state->color.back);
                fprintf(output->bar, "%s", yabr_config.sep_rightside_same);
            }
        }
    }
    state->past_first_entry = 1;
    render_color_no_sep(state, output);
}

static void render_cmd(struct bar_state *state, struct bar_output *output, int button, void *key, const char *cmd)
{
    /* 'key' is a unique id to associate with this cmd. Generally it is a
     * pointer of some sort, as these are guarenteed unqiue for every object */
    fprintf(output->bar, "%%{A%d:0x%lx-%s:}", button, (long)key, cmd);
}

static void render_cmd_end(struct bar_state *state, struct bar_output *output)
{
    fprintf(output->bar, "%%{A}");
}

static void render_right_align(struct bar_state *state, struct bar_output *output)
{
    fprintf(output->bar, "%%{r}");
    state->sep_direction = 1;
}

static void render_left_align(struct bar_state *state, struct bar_output *output)
{
    fprintf(output->bar, "%%{l}");
}

static void render_center_align(struct bar_state *state, struct bar_output *output)
{
    fprintf(output->bar, "%%{c}");
}

static void render_finish(struct bar_state *state, struct bar_output *output)
{
    fputc('\n', output->bar);
    fflush(output->bar);

    state->sep_direction = 0;
    state->past_first_entry = 0;
    state->cur_status_color = 0;
}

static void render_single_status(struct bar_state *state, struct bar_output *output, struct status *status)
{
    int cmd_count = 0, i;
    if (!flag_test(&status->flags, STATUS_VISIBLE))
        return ;

    if (!status->text)
        return ;

    for (i = 0; i < ARRAY_SIZE(status->cmds); i++) {
        if (status->cmds[i].cmd) {
            cmd_count++;
            render_cmd(state, output, i + 1, status, status->cmds[i].cmd);
        }
    }

    fprintf(output->bar, " %s ", status->text);

    for (i = 0; i < cmd_count; i++)
        render_cmd_end(state, output);
}

static void status_render(struct bar_state *state, struct bar_output *output)
{
    int cur_status_color = 0;

    struct status *status, *last_entry = NULL;

    list_foreach_entry(&state->status_list.list, status, status_entry)
        if (flag_test(&status->flags, STATUS_VISIBLE))
            last_entry = status;

    if (!last_entry)
        return ;

    list_foreach_entry(&state->status_list.list, status, status_entry) {
        int is_last = status == last_entry;

        if (flag_test(&status->flags, STATUS_URGENT)) {
            state->color = yabr_config.colors.status_urgent;
        } else {
            if (is_last)
                state->color = yabr_config.colors.status_last;
            else
                state->color = yabr_config.colors.section_cols[cur_status_color];

            cur_status_color = (cur_status_color + 1) % yabr_config.colors.section_count;
        }

        render_color(state, output);

        if (!flag_test(&status->flags, STATUS_VISIBLE))
            continue;

        render_single_status(state, output, status);
    }
}

static void mode_render(struct bar_state *state, struct bar_output *output)
{
    if (!state->mode)
        return ;

    state->color = yabr_config.colors.mode;
    render_color(state, output);

    fprintf(output->bar, " %s ", state->mode);
}

static void title_render(struct bar_state *state, struct bar_output *output)
{
    state->color = yabr_config.colors.title;
    render_color(state, output);
    fprintf(output->bar, " %s ", state->win_title);
}

static void ws_list_render(struct bar_state *state, struct bar_output *output)
{
    char cmdbuf[128];

    render_cmd(state, output, 4, state, "prev");
    render_cmd(state, output, 5, state, "next");

    struct ws *ws;
    list_foreach_entry(&state->ws_list.list, ws, ws_entry) {
        if (strcmp(ws->output, output->name) != 0)
            continue;

        if (flag_test(&ws->flags, WS_URGENT))
            state->color = yabr_config.colors.wsp_urgent;
        else if (flag_test(&ws->flags, WS_FOCUSED))
            state->color = yabr_config.colors.wsp_focused;
        else
            state->color = yabr_config.colors.wsp_unfocused;

        snprintf(cmdbuf, sizeof(cmdbuf), "switch-%s", ws->name);

        render_cmd(state, output, 1, state, cmdbuf);
        render_color(state, output);

        fprintf(output->bar, " %s ", ws->name);

        render_cmd_end(state, output);
    }

    render_cmd_end(state, output);
    render_cmd_end(state, output);
}

/* 
 * Note - internal bar state is reused across multiple outputs. This is fine
 * because the bar is already left in the same state at the end of a render
 */
static void bar_state_render_output(struct bar_state *state, struct bar_output *output)
{
    render_left_align(state, output);
    ws_list_render(state, output);
    mode_render(state, output);
    title_render(state, output);

    state->color = yabr_config.colors.def;
    render_color(state, output);

    if (state->centered
        && flag_test(&state->centered->flags, STATUS_VISIBLE)) {
        render_center_align(state, output);
        state->sep_direction = 1;
        state->color = yabr_config.colors.centered;
        render_color(state, output);

        render_single_status(state, output, state->centered);

        state->sep_direction = 0;
        state->color = yabr_config.colors.def;
        render_color(state, output);
    }

    render_right_align(state, output);
    status_render(state, output);

    state->color = yabr_config.colors.def;

    render_color_no_sep(state, output);
    render_finish(state, output);
}

void bar_state_render(struct bar_state *state)
{
    int i;

    for (i = 0; i < state->output_count; i++)
        bar_state_render_output(state, state->outputs + i);

    return ;
}

