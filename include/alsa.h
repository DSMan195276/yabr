#ifndef INCLUDE_ALSA_H
#define INCLUDE_ALSA_H

#include "render.h"

struct status *alsa_status_create(const char *mix, const char *card);

#endif
