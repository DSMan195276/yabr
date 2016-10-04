#ifndef INCLUDE_I3_H
#define INCLUDE_I3_H

#include <i3ipc-glib/i3ipc-glib.h>

struct bar_state;

struct i3_state {
    i3ipcConnection *conn;
};

int i3_state_setup(struct bar_state *state);
void i3_state_close(struct bar_state *state);

void i3_state_main(struct bar_state *state);
void i3_state_update_title(struct bar_state *state);

#endif
