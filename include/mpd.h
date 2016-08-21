#ifndef INCLUDE_MPD_H
#define INCLUDE_MPD_H

#include "render.h"

void mpdmon_status_add(struct bar_state *, const char *server, int port, int timeout);

#endif
