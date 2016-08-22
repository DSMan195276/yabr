
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>

#include <glib/gprintf.h>

#include "render.h"
#include "bar_config.h"
#include "tasks.h"

struct tasks {
    struct status status;
    struct status test_status;

    struct bar_state *state;
};

struct taskwarrior_task {
    char *description;
    struct tm due;
    time_t due_time;
};

void taskwarrior_task_clear(struct taskwarrior_task *task)
{
    free(task->description);
}

void taskwarrior_task_read(struct taskwarrior_task *task, FILE *file)
{
    char *line;
    ssize_t len;
    size_t buf_len = 0;

    while ((len = getline(&line, &buf_len, file)) != -1) {
        char buf[128];
        if (sscanf(line, "Description %s\n", buf) == 1) {
            task->description = strdup(buf);
        } else if (sscanf(line, "Due %d-%d-%d\n", &task->due.tm_year, &task->due.tm_mon, &task->due.tm_mday) == 3) {
            task->due.tm_year -= 1900;
            task->due.tm_mon -= 1;
            task->due_time = mktime(&task->due);
        }
    }

    return ;
}

static void tasks_update(struct tasks *tasks)
{
    char buf[128];
    FILE *prog;
    int task_due_week_count, test_id;
    int test_comming;

    prog = popen("task due.before:`task calc now + 6d` status:Pending count", "r");
    fscanf(prog, "%d", &task_due_week_count);
    pclose(prog);

    if (!task_due_week_count) {
        flag_clear(&tasks->status.flags, STATUS_VISIBLE);
    } else {
        flag_set(&tasks->status.flags, STATUS_VISIBLE);
        snprintf(buf, sizeof(buf), "Tasks due: %d", task_due_week_count);
        status_change_text(&tasks->status, buf);
    }

    prog = popen("task due.before:`task calc now + 3d` status:Pending +test ids", "r");
    test_comming = fscanf(prog, "%d", &test_id);
    pclose(prog);

    if (test_comming != 1) {
        flag_clear(&tasks->test_status.flags, STATUS_VISIBLE);
    } else {
        struct taskwarrior_task task;
        struct timeval current_day;
        double diff_time;
        int days;

        memset(&task, 0, sizeof(task));

        gettimeofday(&current_day, NULL);

        snprintf(buf, sizeof(buf), "task %d information", test_id);
        prog = popen(buf, "r");
        taskwarrior_task_read(&task, prog);
        pclose(prog);

        diff_time = difftime(task.due_time, current_day.tv_sec);

        days = (int)ceil(diff_time / 60 / 60 / 24);
        snprintf(buf, sizeof(buf), "%s: %d day%s", task.description, days, (days != 1)? "s": "");

        flag_set(&tasks->test_status.flags, STATUS_VISIBLE);
        flag_set(&tasks->test_status.flags, STATUS_URGENT);
        status_change_text(&tasks->test_status, buf);

        taskwarrior_task_clear(&task);
    }
}

static gboolean task_check(gpointer data)
{
    struct tasks *tasks = data;

    fprintf(stderr, "Task update\n");

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

    status_list_add(&state->status_list, &tasks->test_status);
    status_list_add(&state->status_list, &tasks->status);

    /* Check every 5 minutes */
    g_timeout_add_seconds(5 * 60, task_check, tasks);
}

