
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include <glib/gprintf.h>

#include "render.h"
#include "bar_config.h"
#include "datetime.h"

struct datetime {
    struct status status;

    int timeout;
    char *fmt;
};

static void update_datetime(struct datetime *datetime, const time_t *time)
{
    char buf[128];

    strftime(buf, sizeof(buf), datetime->fmt, localtime(time));

    flag_set(&datetime->status.flags, STATUS_VISIBLE);
    status_change_text(&datetime->status,buf);
}

static gboolean time_change(gpointer data)
{
    struct datetime *datetime = data;
    struct timeval current_time;

    gettimeofday(&current_time, NULL);
    update_datetime(datetime, &current_time.tv_sec);

    bar_render_global();

    return TRUE;
}

static void datetime_setup(struct datetime *datetime)
{
    struct timeval current_time;

    gettimeofday(&current_time, NULL);
    update_datetime(datetime, &current_time.tv_sec);

    g_timeout_add_seconds(datetime->timeout, time_change, datetime);
}

struct status *datetime_status_create(const char *fmt, int timeout)
{
    struct datetime *datetime;

    datetime = malloc(sizeof(*datetime));
    memset(datetime, 0, sizeof(*datetime));
    status_init(&datetime->status);
    datetime->fmt = strdup(fmt);
    datetime->timeout = timeout;

    datetime_setup(datetime);

    return &datetime->status;
}

