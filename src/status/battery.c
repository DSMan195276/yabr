
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include <glib/gprintf.h>

#include "status.h"
#include "status_desc.h"
#include "config.h"
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

    char *id;
    int timeout;

    enum power_state state;
    uint64_t minutes_left;
};

static void battery_get_state(struct battery *battery)
{
#define BUF_MAX 128
    char path[PATH_MAX];
    char tmp[BUF_MAX];
    char *line = NULL;
    size_t bufsize = 0, len;
    FILE *state;
    int64_t full_capacity = -1, remain_capacity = -1;
    int64_t current_rate = -1;

    snprintf(path, sizeof(path), "/sys/class/power_supply/%s/uevent", battery->id);

    state = fopen(path, "r");
    if (!state) {
        battery->state = POWER_STATE_NO_BATTERY;
        return ;
    }

    while ((len = getline(&line, &bufsize, state)) != -1) {
        if (sscanf(line, "POWER_SUPPLY_STATUS=%"Q(BUF_MAX)"s\n", tmp) == 1) {
            if (strcmp(tmp, "Charging") == 0)
                battery->state = POWER_STATE_CHARGING;
            else if (strcmp(tmp, "Full") == 0)
                battery->state = POWER_STATE_CHARGED;
            else if (strcmp(tmp, "Discharging") == 0)
                battery->state = POWER_STATE_DISCHARGING;
            else /* Various states like "Unknown" are considered charging */
                battery->state = POWER_STATE_CHARGING;

        } else if (sscanf(line, "POWER_SUPPLY_CHARGE_NOW=%ld\n", &remain_capacity) == 1) {
            continue;
        } else if (sscanf(line, "POWER_SUPPLY_CHARGE_FULL=%ld\n", &full_capacity) == 1) {
            continue;
        } else if (sscanf(line, "POWER_SUPPLY_CURRENT_NOW=%ld\n", &current_rate) == 1) {
            continue;
        }
    }

    if (battery->state == POWER_STATE_CHARGED)
        goto cleanup_state;

    if (current_rate == -1 || full_capacity == -1 || remain_capacity == -1) {
        battery->minutes_left = 0;
        goto cleanup_state;
    }

    if (battery->state == POWER_STATE_CHARGING && current_rate == 0)
        battery->state = POWER_STATE_CHARGED;

    switch (battery->state) {
    case POWER_STATE_CHARGING:
        if (current_rate)
            battery->minutes_left = ((full_capacity - remain_capacity) * 60) / current_rate;
        else
            battery->minutes_left = 0;
        break;

    case POWER_STATE_DISCHARGING:
        if (current_rate)
            battery->minutes_left = (remain_capacity * 60) / current_rate;
        else
            battery->minutes_left = 0;
        dbgprintf("Battery: Remain cap: %ld, current: %ld, minutes: %ld\n", remain_capacity, current_rate, battery->minutes_left);
        break;

    case POWER_STATE_CHARGED:
    case POWER_STATE_NO_BATTERY:
        break;
    }

  cleanup_state:
    if (line)
        free(line);
    fclose(state);
    return ;
#undef BUF_MAX
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
    bar_render_global();

    return TRUE;
}

static void battery_setup(struct battery *battery)
{
    flag_set(&battery->status.flags, STATUS_VISIBLE);

    battery_get_state(battery);
    battery_render(battery);

    g_timeout_add_seconds(battery->timeout, battery_check_status, battery);
}

static struct status *battery_status_create(const char *id, int timeout)
{
    struct battery *battery;

    battery = malloc(sizeof(*battery));
    memset(battery, 0, sizeof(*battery));
    status_init(&battery->status);
    battery->id = strdup(id);
    battery->timeout = timeout;
    battery_setup(battery);

    return &battery->status;
}

static struct status *battery_create(struct status_config_item *list)
{
    const char *id = status_config_get_str(list, "id");
    int timeout = status_config_get_int(list, "timeout");
    return battery_status_create(id, timeout);
}

const struct status_description battery_status_description = {
    .name = "battery",
    .items = (struct status_config_item []) {
        STATUS_CONFIG_ITEM_STR("id", "BAT0"),
        STATUS_CONFIG_ITEM_INT("timeout", 60 * 5),
        STATUS_CONFIG_END(),
    },
    .status_create = battery_create,
};

