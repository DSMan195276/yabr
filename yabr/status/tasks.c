
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include <glib/gprintf.h>

#include "render.h"
#include "bar_config.h"
#include "tasks.h"

struct tasks {
    struct status status;
    struct bar_state *state;
};

static void tasks_update(struct tasks *tasks)
{
    char buf[128];
    FILE *prog;
    int task_due_week_count;

    prog = popen("task due.before:`task calc now + 6d` status:Pending count", "r");
    fscanf(prog, "%d", &task_due_week_count);
    pclose(prog);

    if (!task_due_week_count) {
        flag_clear(&tasks->status.flags, STATUS_VISIBLE);
    } else {
        fprintf(stderr, "Tasks: %d\n", task_due_week_count);
        flag_set(&tasks->status.flags, STATUS_VISIBLE);
        snprintf(buf, sizeof(buf), "Tasks due: %d", task_due_week_count);
        status_change_text(&tasks->status, buf);
    }
}

static gboolean task_check(gpointer data)
{
    struct tasks *tasks = data;

    tasks_update(tasks);
    bar_state_render(tasks->state);

    return TRUE;
}

void tasks_status_add(struct bar_state *state)
{
    struct tasks *tasks;

    tasks = malloc(sizeof(*tasks));
    memset(tasks, 0, sizeof(*tasks));
    status_init(&tasks->status);
    tasks->state = state;

    tasks_update(tasks);

    status_list_add(&state->status_list, &tasks->status);

    /* Check every 5 minutes */
    g_timeout_add_seconds(60000 * 5, task_check, tasks);
}

