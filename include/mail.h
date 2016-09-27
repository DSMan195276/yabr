#ifndef INCLUDE_MAIL_H
#define INCLUDE_MAIL_H

#include "render.h"

struct status *mail_status_create(const char *name, const char *mail_dir, int timeout);

#endif
