
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
    union {
        struct {
            struct status date;
            struct status time;
        };
        struct status datetime;
    };

    struct bar_state *bar_state;

    int date_timeout;
    char *datefmt, *timefmt;
    int flags;
};

static void update_time_date(struct datetime *datetime, const time_t *time)
{
    char full[128];
    char date_buf[128];
    char time_buf[128];

    strftime(time_buf, sizeof(time_buf), datetime->timefmt, localtime(time));
    strftime(date_buf, sizeof(date_buf), datetime->datefmt, localtime(time));

    if (flag_set(&datetime->flags, DATETIME_SPLIT)) {
        status_change_text(&datetime->date, date_buf);
        flag_set(&datetime->date.flags, STATUS_VISIBLE);

        status_change_text(&datetime->time, time_buf);
        flag_set(&datetime->time.flags, STATUS_VISIBLE);
    } else {
        snprintf(full, sizeof(full), "%s " BAR_SEP_RIGHTSIDE_SAME " %s", date_buf, time_buf);
        status_change_text(&datetime->datetime, full);
        flag_set(&datetime->datetime.flags, STATUS_VISIBLE);
    }
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

void datetime_status_add(struct bar_state *state, const char *datefmt, const char *timefmt, int date_timeout, int flags)
{
    struct datetime *datetime = malloc(sizeof(*datetime));

    memset(datetime, 0, sizeof(*datetime));
    if (flag_set(&flags, DATETIME_SPLIT)) {
        status_init(&datetime->date);
        status_init(&datetime->time);
    } else {
        status_init(&datetime->datetime);
    }
    datetime->bar_state = state;
    datetime->flags = flags;
    datetime->date_timeout = date_timeout;
    datetime->datefmt = strdup(datefmt);
    datetime->timefmt = strdup(timefmt);

    datetime_setup(datetime);

    if (flag_set(&flags, DATETIME_SPLIT)) {
        status_list_add(&state->status_list, &datetime->time);
        status_list_add(&state->status_list, &datetime->date);
    } else {
        status_list_add(&state->status_list, &datetime->datetime);
    }
}

