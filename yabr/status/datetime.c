
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
    struct status time;
    struct status date;

    struct bar_state *bar_state;

    int date_timeout;
    char *datefmt, *timefmt;
};

static void update_time_date(struct datetime *datetime, const time_t *time)
{
    char buf[128];
    strftime(buf, sizeof(buf), datetime->datefmt, localtime(time));

    status_change_text(&datetime->date, buf);
    flag_set(&datetime->date.flags, STATUS_VISIBLE);

    strftime(buf, sizeof(buf), datetime->timefmt, localtime(time));

    status_change_text(&datetime->time, buf);
    flag_set(&datetime->time.flags, STATUS_VISIBLE);
}

static gboolean time_change(gpointer data)
{
    struct datetime *datetime = data;
    struct timeval current_time;

    gettimeofday(&current_time, NULL);
    update_time_date(datetime, &current_time.tv_sec);

    bar_state_render(datetime->bar_state);

    return TRUE;
}

static void datetime_setup(struct datetime *datetime)
{
    struct timeval current_time;

    gettimeofday(&current_time, NULL);
    update_time_date(datetime, &current_time.tv_sec);

    g_timeout_add_seconds(datetime->date_timeout, time_change, datetime);
}

void datetime_status_add(struct bar_state *state, const char *datefmt, const char *timefmt, int date_timeout)
{
    struct datetime *datetime = malloc(sizeof(*datetime));

    memset(datetime, 0, sizeof(*datetime));
    status_init(&datetime->time);
    status_init(&datetime->date);
    datetime->bar_state = state;
    datetime->date_timeout = date_timeout;
    datetime->datefmt = strdup(datefmt);
    datetime->timefmt = strdup(timefmt);

    datetime_setup(datetime);

    status_list_add(&state->status_list, &datetime->time);
    status_list_add(&state->status_list, &datetime->date);
}

