
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "list.h"
#include "render.h"
#include "config.h"

#include "alsa.h"
#include "battery.h"
#include "datetime.h"
#include "mail.h"
#include "mpd.h"
#include "tasks.h"
#include "wireless.h"

struct yabr_config yabr_config = {
    .debug = NULL,
    .state = BAR_STATE_INIT(yabr_config.state),
    .descs = {
        [DESC_ALSA] = &alsa_status_description,
        [DESC_BATTERY] = &battery_status_description,
        [DESC_DATETIME] = &datetime_status_description,
        [DESC_MAIL] = &mail_status_description,
        [DESC_MPD] = &mpdmon_status_description,
        [DESC_TASKS] = &tasks_status_description,
        [DESC_WIRELESS] = &wireless_status_description,
    },
};

