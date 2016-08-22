#ifndef INCLUDE_BATTERY_H
#define INCLUDE_BATTERY_H

#include "render.h"

struct status *battery_status_create(const char *battery_id, int battery_timeout);

#endif
