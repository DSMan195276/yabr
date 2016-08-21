#ifndef INCLUDE_DATETIME_H
#define INCLUDE_DATETIME_H

#include "render.h"

void datetime_status_add(struct bar_state *state, const char *datefmt, const char *timefmt, int date_timeout);

#endif
