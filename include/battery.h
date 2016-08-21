#ifndef INCLUDE_BATTERY_H
#define INCLUDE_BATTERY_H

#include "render.h"

void battery_status_add(struct bar_state *, const char *battery_id, int battery_timeout);

#endif
