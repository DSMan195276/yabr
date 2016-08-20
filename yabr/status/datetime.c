
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include <glib/gprintf.h>

#include "render.h"
#include "bar_config.h"

struct status time_status = STATUS_INIT(time_status);
struct status date_status = STATUS_INIT(date_status);

static void update_time_date(const time_t *time)
{
    char buf[128];
    strftime(buf, sizeof(buf), DATE_FORMAT, localtime(time));

    if (date_status.text)
        free(date_status.text);

    date_status.text = strdup(buf);
    flag_set(&date_status.flags, STATUS_VISIBLE);

    strftime(buf, sizeof(buf), TIME_FORMAT, localtime(time));

    if (time_status.text)
        free(time_status.text);

    time_status.text = strdup(buf);
    flag_set(&time_status.flags, STATUS_VISIBLE);
}

static gboolean time_change(gpointer data)
{
    struct timeval current_time;

    gettimeofday(&current_time, NULL);
    update_time_date(&current_time.tv_sec);

    bar_state_render(&bar_state);

    return TRUE;
}

void date_time_setup(i3ipcConnection *conn)
{
    struct timeval current_time;

    status_list_add(&bar_state.status_list, &time_status);
    status_list_add(&bar_state.status_list, &date_status);

    gettimeofday(&current_time, NULL);
    update_time_date(&current_time.tv_sec);

    g_timeout_add(DATE_TIMEOUT, time_change, NULL);
}

