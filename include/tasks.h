#ifndef INCLUDE_TASKS_H
#define INCLUDE_TASKS_H

#include "render.h"

struct status *tasks_status_create(int timeout);
struct status *tasks_test_status_create(int timeout);

#endif
