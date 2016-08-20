
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include <glib/gprintf.h>

#include "status.h"
#include "render.h"
#include "bar_config.h"

#define BATTERY_PATH_STATE "/proc/acpi/battery/"BATTERY_USE"/state"

struct status battery_status = STATUS_INIT(battery_status);

enum power_state {
    POWER_STATE_NO_BATTERY,
    POWER_STATE_CHARGING,
    POWER_STATE_CHARGED,
    POWER_STATE_DISCHARGING
};

struct battery {
    enum power_state state;
    uint64_t minutes_left;
};

static struct battery bat;

static void battery_get_state(struct battery *battery)
{
    char tmp[128];
    char *line = NULL;
    size_t bufsize = 0, len;
    FILE *state;
    int64_t rate = -1, remain_capacity = -1;

    state = fopen(BATTERY_PATH_STATE, "r");

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
    if (battery_status.text)
        free(battery_status.text);

    switch (battery->state) {
    case POWER_STATE_NO_BATTERY:
        flag_set(&battery_status.flags, STATUS_VISIBLE);
        battery_status.text = strdup("No Battery");
        break;

    case POWER_STATE_CHARGED:
        flag_set(&battery_status.flags, STATUS_VISIBLE);
        battery_status.text = strdup("Battery: Charged");
        break;

    case POWER_STATE_DISCHARGING:
        flag_set(&battery_status.flags, STATUS_VISIBLE);
        snprintf(tmp, sizeof(tmp), "Battery: Discharging - %02ld:%02ld", battery->minutes_left / 60, battery->minutes_left % 60);
        battery_status.text = strdup(tmp);
        break;

    case POWER_STATE_CHARGING:
        flag_set(&battery_status.flags, STATUS_VISIBLE);
        battery_status.text = strdup("Battery: Charging");
        break;
    }

}

static gboolean battery_check_status(gpointer data)
{
    battery_get_state(&bat);
    battery_render(&bat);
    bar_state_render(&bar_state);

    return TRUE;
}

void battery_setup(i3ipcConnection *conn)
{
    status_list_add(&bar_state.status_list, &battery_status);

    battery_get_state(&bat);
    battery_render(&bat);

    g_timeout_add(10000, battery_check_status, NULL);
}

