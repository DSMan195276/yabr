#ifndef INCLUDE_STATUS_DESC
#define INCLUDE_STATUS_DESC

#include <string.h>
#include <stdlib.h>

#include "flag.h"
#include "status.h"

enum status_config_item_type {
    STATUS_CONFIG_INT,
    STATUS_CONFIG_STRING,
};

struct status_config_item {
    const char *name;
    int flags;
    enum status_config_item_type type;
    union {
        char *str;
        int i;
    };
};

enum {
    STATUS_CONFIG_FLAG_CHANGE,
};

#define STATUS_CONFIG_ITEM_STR(nam, def) \
    { \
        .name = nam, \
        .type = STATUS_CONFIG_STRING, \
        .str = def \
    }

#define STATUS_CONFIG_ITEM_INT(nam, def) \
    { \
        .name = nam, \
        .type = STATUS_CONFIG_INT, \
        .i = def \
    }

#define STATUS_CONFIG_END() \
    { .name = NULL }

struct status_description {
    const char *name;

    /* Terminated via .name = NULL */
    struct status_config_item *items;

    struct status *(*status_create) (struct status_config_item *);
};

struct status_config_item *status_config_get(struct status_config_item *list, const char *id);
const char *status_config_get_str(struct status_config_item *list, const char *id);
int status_config_get_int(struct status_config_item *list, const char *id);

int status_config_set_str(struct status_config_item *list, const char *id, const char *value);
int status_config_set_int(struct status_config_item *list, const char *id, int value);

int status_config_item_set_str(struct status_config_item *item, const char *value);
int status_config_item_set_int(struct status_config_item *item, int value);

int status_config_list_count(struct status_config_item *list);
void status_config_list_clear(struct status_config_item *list);

#endif
