#ifndef INCLUDE_MPD_H
#define INCLUDE_MPD_H

#include "render.h"

struct status *mpdmon_status_create(const char *server, int port, int timeout);

#endif
