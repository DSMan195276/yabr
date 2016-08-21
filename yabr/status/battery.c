
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include <glib/gprintf.h>

#include "status.h"
#include "render.h"
#include "battery.h"
#include "bar_config.h"

enum power_state {
    POWER_STATE_NO_BATTERY,
    POWER_STATE_CHARGING,
    POWER_STATE_CHARGED,
    POWER_STATE_DISCHARGING
};

struct battery {
    struct status status;
    struct bar_state *bar_state;

    char *id;
    int timeout;

    enum power_state state;
    uint64_t minutes_left;
};

static void battery_get_state(struct battery *battery)
{
    char path[PATH_MAX];
    char tmp[128];
    char *line = NULL;
    size_t bufsize = 0, len;
    FILE *state;
    int64_t rate = -1, remain_capacity = -1;

    snprintf(path, sizeof(path), "/proc/acpi/battery/%s/state", battery->id);

    state = fopen(path, "r");
    if (!state)
        return ;

    while ((len = getline(&line, &bufsize, state)) != -1) {
        if (sscanf(line, "present: %s\n", tmp) == 1) {
            if (strcmp(tmp, "no") == 0) {
                battery->state = POWER_STATE_NO_BATTERY;
                goto cleanup_state;
            }
        } else if (sscanf(line, "charging state: %s\n", tmp) == 1) {
            if (strcmp(tmp, "charged") == 0)
                battery->state = POWER_STATE_CHARGED;
            else if (strcmp(tmp, "charging") == 0)
                battery->state = POWER_STATE_CHARGING;
            else if (strcmp(tmp, "discharging") == 0)
                battery->state = POWER_STATE_DISCHARGING;
        } else if (sscanf(line, "present rate: %ld mWh\n", &rate) == 1) {
            continue;
        } else if (sscanf(line, "remaining capacity: %ld mW\n", &remain_capacity) == 1) {
            continue;
        }
    }

    if (rate > 0 && remain_capacity != -1)
        battery->minutes_left = (remain_capacity * 60) / rate;
    else
        battery->minutes_left = 0;

  cleanup_state:
    if (line)
        free(line);
    fclose(state);
    return ;
}

static void battery_render(struct battery *battery)
{
    char tmp[128];

    switch (battery->state) {
    case POWER_STATE_NO_BATTERY:
        status_change_text(&battery->status, "No Battery");
        break;

    case POWER_STATE_CHARGED:
        status_change_text(&battery->status, "Battery: Charged");
        break;

    case POWER_STATE_DISCHARGING:
        snprintf(tmp, sizeof(tmp), "Battery: Discharging - %02ld:%02ld", battery->minutes_left / 60, battery->minutes_left % 60);
        status_change_text(&battery->status, tmp);
        break;

    case POWER_STATE_CHARGING:
        status_change_text(&battery->status, "Battery: Charging");
        break;
    }

}

static gboolean battery_check_status(gpointer data)
{
    struct battery *battery = data;
    battery_get_state(battery);
    battery_render(battery);
    bar_state_render(battery->bar_state);

    return TRUE;
}

static void battery_setup(struct battery *battery)
{
    flag_set(&battery->status.flags, STATUS_VISIBLE);

    battery_get_state(battery);
    battery_render(battery);

    g_timeout_add(battery->timeout, battery_check_status, battery);
}

void battery_status_add(struct bar_state *state, const char *battery_id, int battery_timeout)
{
    struct battery *battery;

    battery = malloc(sizeof(*battery));

    memset(battery, 0, sizeof(*battery));
    status_init(&battery->status);

    battery->id = strdup(battery_id);
    battery->timeout = battery_timeout;
    battery->bar_state = state;
    battery_setup(battery);

    status_list_add(&state->status_list, &battery->status);

    return ;
}

