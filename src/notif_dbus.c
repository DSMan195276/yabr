
#include <stdio.h>
#include <stdlib.h>

#include <glib/gprintf.h>
#include <dbus/dbus.h>

#include "config.h"
#include "notif_dbus.h"

static gboolean notify_timeout(gpointer data)
{
    struct notif_dbus *dbus = data;

    dbus->msg_timeout_count--;
    if (dbus->msg_timeout_count == 0) {
        dbus_message_unref(dbus->cur_message);
        dbus->cur_message = NULL;

        dbus->appname = NULL;
        dbus->summary = NULL;
        dbus->body = NULL;
        dbus->expires = 0;

        bar_render_global();
    }

    return FALSE;
}

static void setup_notify(struct notif_dbus *dbus)
{
    DBusMessage *reply;
    DBusMessageIter args;
    const char *appname;
    const char *nid;
    const char *icon;
    const char *summary;
    const char *body;
    int expires = -1;

    dbus->serial++;

    dbus_message_iter_init(dbus->cur_message, &args);
    dbus_message_iter_get_basic(&args, &appname);
    dbus_message_iter_next(&args);
    dbus_message_iter_get_basic(&args, &nid);
    dbus_message_iter_next(&args);
    dbus_message_iter_get_basic(&args, &icon);
    dbus_message_iter_next(&args);
    dbus_message_iter_get_basic(&args, &summary);
    dbus_message_iter_next(&args);
    dbus_message_iter_get_basic(&args, &body);
    dbus_message_iter_next(&args);
    dbus_message_iter_next(&args);
    dbus_message_iter_next(&args);
    dbus_message_iter_get_basic(&args, &expires);

    dbus->appname = appname;
    dbus->summary = summary;
    dbus->body = body;
    dbus->expires = expires;

    dbgprintf("Notification %s: %s -> %s\n", appname, summary, body);

    reply = dbus_message_new_method_return(dbus->cur_message);

    dbus_message_iter_init_append(reply, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &nid)
            || !dbus_connection_send(dbus->conn, reply, &dbus->serial))
        return ;

    dbus_message_unref(reply);
}

static void get_server_information(struct notif_dbus *dbus, DBusMessage *msg)
{
    DBusMessage *reply;
    DBusMessageIter args;

    const char *info[4] = {"yabr", "yabr", "2016", "2016"};
    dbus->serial++;


    reply = dbus_message_new_method_return(msg);

    dbus_message_iter_init_append(reply, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, info)
            || !dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, info + 1)
            || !dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, info + 2)
            || !dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, info + 3)
            || !dbus_connection_send(dbus->conn, reply, &dbus->serial))
        return ;

    dbus_message_unref(reply);
}

static void get_capabilities(struct notif_dbus *dbus, DBusMessage *msg)
{
    DBusMessage *reply;
    DBusMessageIter args;
    DBusMessageIter subargs;

    const char *caps[1] = {"body"};

    dbus->serial++;

    reply = dbus_message_new_method_return(msg);
    if (!reply)
        return ;

    dbus_message_iter_init_append(reply, &args);

    if (!dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING, &subargs)
            || !dbus_message_iter_append_basic(&subargs, DBUS_TYPE_STRING, caps)
            || !dbus_message_iter_close_container(&args, &subargs)
            || !dbus_connection_send(dbus->conn, reply, &dbus->serial))
        return ;

    dbus_message_unref(reply);
}

static void setup_timeout(struct notif_dbus *dbus)
{
    g_timeout_add(dbus->expires, notify_timeout, dbus);
    dbus->msg_timeout_count++;
}

static gboolean dbus_handle(GIOChannel *gio, GIOCondition condition, gpointer data)
{
    struct notif_dbus *dbus = data;
    DBusMessage *msg;

    dbus_connection_read_write(dbus->conn, 0);
    while ((msg = dbus_connection_pop_message(dbus->conn)) != NULL) {

        if (dbus_message_is_method_call(msg, "org.freedesktop.Notifications", "Notify")) {
            if (dbus->cur_message)
                dbus_message_unref(dbus->cur_message);

            dbus->cur_message = msg;

            setup_notify(dbus);
            setup_timeout(dbus);
            bar_render_global();
            continue;
        } else if (dbus_message_is_method_call(msg, "org.freedesktop.Notifications", "GetCapabilities")) {
            get_capabilities(dbus, msg);
        } else if (dbus_message_is_method_call(msg, "org.freedesktop.Notifications", "CloseNotification")) {
        } else if (dbus_message_is_method_call(msg, "org.freedesktop.Notifications", "GetServerInformation")) {
            get_server_information(dbus, msg);
        }

        dbus_message_unref(msg);
    }

    return TRUE;
}


int notif_dbus_open(struct notif_dbus *dbus)
{
    DBusError err;
    GIOChannel *channel;
    int ret = 0;
    int fd;

    dbus_error_init(&err);

    dbus->conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err) || !dbus->conn) {
        dbgprintf("Error: Unable to connect to DBus\n");
        ret = 1;
        goto cleanup;
    }

    ret = dbus_bus_request_name(dbus->conn, "org.freedesktop.Notifications", 0, &err);
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
        dbgprintf("Error: Unable to aquire name - Is another Notification daemon running?\n");
        ret = 1;
        goto cleanup;
    }

    ret = dbus_connection_get_unix_fd(dbus->conn, &fd);
    ret = !ret;

    dbgprintf("Dbus connection fd: %d\n", fd);

    channel = g_io_channel_unix_new(fd);
    g_io_add_watch(channel, G_IO_IN, dbus_handle, dbus);

cleanup:
    dbus_error_free(&err);
    return ret;

}

void notif_dbus_close(struct notif_dbus *dbus)
{

}

