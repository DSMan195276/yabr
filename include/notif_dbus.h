#ifndef INCLUDE_NOTIF_DBUS_H
#define INCLUDE_NOTIF_DBUS_H

#include <stdint.h>

#include <glib/gprintf.h>
#include <dbus/dbus.h>

#include "flag.h"

struct notif_dbus {
    DBusConnection *conn;

    uint32_t flags;

    DBusMessage *cur_message;

    /* Counts the number of timers currenlty being run. If one message replaces
     * the current one, it's timer still runs. This count keeps that initial
     * timer from closing the current message */
    uint32_t msg_timeout_count;

    uint32_t serial;

    const char *appname, *summary, *body;
    uint32_t expires;
};

int notif_dbus_open(struct notif_dbus *);
void notif_dbus_close(struct notif_dbus *);

#endif
