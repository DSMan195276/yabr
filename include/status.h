#ifndef INCLUDE_STATUS_H
#define INCLUDE_STATUS_H

#include <i3ipc-glib/i3ipc-glib.h>

void date_time_setup(i3ipcConnection *conn);
void battery_setup(i3ipcConnection *conn);
void alsa_setup(i3ipcConnection *conn);
void mpdmon_setup(i3ipcConnection *conn);
void wireless_setup(i3ipcConnection *conn);

#endif
