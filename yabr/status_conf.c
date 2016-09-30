
#include <string.h>
#include <stdlib.h>

#include "status.h"
#include "status_desc.h"

struct status_config_item *status_config_get(struct status_config_item *list, const char *id)
{
    struct status_config_item *config;
    for (config = list; config->name; config++)
        if (strcmp(config->name, id) == 0)
            return config;

    return NULL;
}

const char *status_config_get_str(struct status_config_item *list, const char *id)
{
    struct status_config_item *config = status_config_get(list, id);
    if (config)
        return config->str;
    else
        return NULL;
}

int status_config_get_int(struct status_config_item *list, const char *id)
{
    struct status_config_item *config = status_config_get(list, id);
    if (config)
        return config->i;
    else
        return 0;
}

int status_config_set_str(struct status_config_item *list, const char *id, const char *value)
{
    struct status_config_item *config = status_config_get(list, id);
    if (!config || config->type != STATUS_CONFIG_STRING)
        return -1;

    if (flag_test(&config->flags, STATUS_CONFIG_FLAG_CHANGE))
        free(config->str);
    else
        flag_set(&config->flags, STATUS_CONFIG_FLAG_CHANGE);

    config->str = strdup(value);
    return 0;
}

int status_config_set_int(struct status_config_item *list, const char *id, int value)
{
    struct status_config_item *config = status_config_get(list, id);
    if (!config || config->type != STATUS_CONFIG_INT)
        return -1;

    flag_set(&config->flags, STATUS_CONFIG_FLAG_CHANGE);
    config->i = value;
    return 0;
}

int status_config_list_count(struct status_config_item *list)
{
    struct status_config_item *config;
    int count = 0;
    for (config = list; config->name; config++)
        count++;

    return count;
}

void status_config_list_clear(struct status_config_item *list)
{
    struct status_config_item *config;
    for (config = list; config->name; config++) {
        if (flag_test(&config->flags, STATUS_CONFIG_FLAG_CHANGE)) {
            switch (config->type) {
            case STATUS_CONFIG_STRING:
                free(config->str);
                break;

            case STATUS_CONFIG_INT:
                break;
            }
        }
    }
}

