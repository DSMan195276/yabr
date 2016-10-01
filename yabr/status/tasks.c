
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>

#include <glib/gprintf.h>

#include "render.h"
#include "bar_config.h"
#include "status_desc.h"
#include "config.h"
#include "tasks.h"

struct tasks {
    struct status status;
    char *days;
    char *tag;
};

struct taskwarrior_task {
    char *description;

    struct tm due;
    time_t due_time;
};

static void taskwarrior_task_clear(struct taskwarrior_task *task)
{
    free(task->description);
}

static void taskwarrior_task_read(struct taskwarrior_task *task, FILE *file)
{
    char *line = NULL;
    ssize_t len;
    size_t buf_len = 0;

    while ((len = getline(&line, &buf_len, file)) != -1) {
        char buf[128];
        if (sscanf(line, "Description %[^\n]", buf) == 1) {
            task->description = strdup(buf);
        } else if (sscanf(line, "Due %d-%d-%d\n", &task->due.tm_year, &task->due.tm_mon, &task->due.tm_mday) == 3) {
            task->due.tm_year -= 1900;
            task->due.tm_mon -= 1;
            task->due_time = mktime(&task->due);
        }
    }

    if (line)
        free(line);

    return ;
}

static void tasks_update(struct tasks *tasks)
{
    char buf[128];
    FILE *prog;
    int task_due_week_count;

    snprintf(buf, sizeof(buf), "task due.before:`task calc now + %s` status:Pending %c%s count",
            tasks->days, (tasks->tag)? '+': ' ', (tasks->tag)? tasks->tag: "");

    prog = popen(buf, "r");
    fscanf(prog, "%d", &task_due_week_count);
    pclose(prog);

    dbgprintf("Tasks: %d\n", task_due_week_count);
    dbgprintf("Tasks cmd: %s\n", buf);

    if (!task_due_week_count) {
        flag_clear(&tasks->status.flags, STATUS_VISIBLE);
    } else {
        flag_set(&tasks->status.flags, STATUS_VISIBLE);
        snprintf(buf, sizeof(buf), "Tasks due: %d", task_due_week_count);
        status_change_text(&tasks->status, buf);
    }
}

static gboolean task_check(gpointer data)
{
    struct tasks *tasks = data;

    tasks_update(tasks);
    bar_render_global();

    return TRUE;
}

static struct status *tasks_status_create(const char *days, const char *tag, int timeout)
{
    struct tasks *tasks;

    tasks = malloc(sizeof(*tasks));
    memset(tasks, 0, sizeof(*tasks));
    status_init(&tasks->status);
    tasks->days = strdup(days);
    if (tag)
        tasks->tag = strdup(tag);

    tasks_update(tasks);

    g_timeout_add_seconds(timeout, task_check, tasks);

    return &tasks->status;
}

static struct status *tasks_create(struct status_config_item *list)
{
    const char *days = status_config_get_str(list, "days");
    const char *tag = status_config_get_str(list, "tag");
    int timeout = status_config_get_int(list, "timeout");

    return tasks_status_create(days, tag, timeout);
}

const struct status_description tasks_status_description = {
    .name = "tasks",
    .items = (struct status_config_item []) {
        STATUS_CONFIG_ITEM_INT("timeout", 60),
        STATUS_CONFIG_ITEM_STR("days", "5d"),
        STATUS_CONFIG_ITEM_STR("tag", NULL),
        STATUS_CONFIG_END(),
    },
    .status_create = tasks_create,
};

