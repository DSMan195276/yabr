
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <mpd/client.h>
#include <mpd/status.h>
#include <mpd/entity.h>
#include <mpd/tag.h>
#include <mpd/message.h>

#include "status.h"
#include "render.h"
#include "bar_config.h"
#include "config.h"
#include "status_desc.h"
#include "mpd.h"

/* Note: Some things start as 'modmon' instead of 'mpd' to avoid name conflicts */

struct mpdmon {
    struct status status;
    struct mpd_connection *conn;

    char *server;
    int port;
    int timeout;
};

#define P_READ 0
#define P_WRITE 1

static void song_to_string(struct mpd_song *song, char *buf, size_t buf_size)
{
    const char *title, *artist;

    title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
    if (title && artist)
        snprintf(buf, buf_size, "%s by %s", title, artist);
    else
        snprintf(buf, buf_size, "%s", mpd_song_get_uri(song));

    return ;
}

static void mpdmon_update_status(struct mpdmon *mpdmon)
{
    char buf[128];
    char song_name[128];
    struct mpd_song *song;
    struct mpd_status *status;
    const char *state;

    status = mpd_run_status(mpdmon->conn);
    if (!status)
        goto cleanup_status;

    song = mpd_run_current_song(mpdmon->conn);

    switch (mpd_status_get_state(status)) {
    case MPD_STATE_PLAY:
        state = "";
        goto make_title;

    case MPD_STATE_PAUSE:
        state = "Paused - ";
        goto make_title;

    make_title:
        song_to_string(song, song_name, sizeof(song_name));
        snprintf(buf, sizeof(buf), "%s%s",
                state,
                song_name);
        status_change_text(&mpdmon->status, buf);
        flag_set(&mpdmon->status.flags, STATUS_VISIBLE);
        dbgprintf("mpd: status: %s\n", buf);
        break;

    case MPD_STATE_UNKNOWN:
    case MPD_STATE_STOP:
        flag_clear(&mpdmon->status.flags, STATUS_VISIBLE);
        dbgprintf("mpd: Unknown status\n");
        break;
    }

    if (song)
        mpd_song_free(song);

  cleanup_status:
    mpd_status_free(status);
    return ;
}

static gboolean mpdmon_timeout_check(gpointer data);

static void mpdmon_parse_idle(struct mpdmon *mpdmon, int idle)
{
    /*
     * We check for MPD_IDLE_QUEUE as well as MPD_IDLE_PLAYER because the name
     * of the current song may have changed. mopidy sends MPD_IDLE_QUEUE when
     * that happens, and no MPD_IDLE_PLAYER.
     */
    if (idle & MPD_IDLE_PLAYER || idle & MPD_IDLE_QUEUE) {
        mpdmon_update_status(mpdmon);
        bar_render_global();
    }
}

static gboolean mpd_handle_idle(GIOChannel *gio, GIOCondition condition, gpointer data)
{
    struct mpdmon *mpdmon = data;
    int idle;

    idle = mpd_recv_idle(mpdmon->conn, false);

    if (idle == 0) {
        mpd_connection_free(mpdmon->conn);
        mpdmon->conn = NULL;
        g_timeout_add_seconds(mpdmon->timeout, mpdmon_timeout_check, mpdmon);

        flag_clear(&mpdmon->status.flags, STATUS_VISIBLE);
        bar_render_global();
        return FALSE;
    }

    mpdmon_parse_idle(mpdmon, idle);

    mpd_send_idle(mpdmon->conn);
    return TRUE;
}

static void mpdmon_connection_check_up(struct mpdmon *mpdmon)
{
    mpdmon->conn = mpd_connection_new(mpdmon->server, mpdmon->port, 0);

    if (mpd_connection_get_error(mpdmon->conn) != MPD_ERROR_SUCCESS) {
        mpd_connection_free(mpdmon->conn);
        mpdmon->conn = NULL;
        return ;
    }
}

static void mpdmon_connection_up(struct mpdmon *mpdmon)
{
    GIOChannel *gio_read;

    gio_read = g_io_channel_unix_new(mpd_connection_get_fd(mpdmon->conn));
    g_io_add_watch(gio_read, G_IO_IN, mpd_handle_idle, mpdmon);

    mpdmon_update_status(mpdmon);
    mpd_send_idle(mpdmon->conn);
}

static gboolean mpdmon_timeout_check(gpointer data)
{
    struct mpdmon *mpdmon = data;

    mpdmon_connection_check_up(mpdmon);
    if (mpdmon->conn) {
        mpdmon_connection_up(mpdmon);
        bar_render_global();
        return FALSE;
    }

    return TRUE;
}

static void mpdmon_handle_cmd_toggle(struct status *status, struct cmd_entry *cmd, const char *args)
{
    struct mpdmon *mpdmon = container_of(status, struct mpdmon, status);

    dbgprintf("mpd: Toggle pause cmd\n");

    /* Note: mpd is always left in idle mode, so we exit idle, perform our
     * action, and then enable idle */
    if (mpdmon->conn) {
        int idle = mpd_run_noidle(mpdmon->conn);

        mpdmon_parse_idle(mpdmon, idle);
        mpd_run_toggle_pause(mpdmon->conn);

        mpd_send_idle(mpdmon->conn);
    }
}

static void mpdmon_handle_cmd_next(struct status *status, struct cmd_entry *cmd, const char *args)
{
    struct mpdmon *mpdmon = container_of(status, struct mpdmon, status);

    dbgprintf("mpd: Next cmd\n");

    /* Note: mpd is always left in idle mode, so we exit idle, perform our
     * action, and then enable idle */
    if (mpdmon->conn) {
        int idle = mpd_run_noidle(mpdmon->conn);

        mpdmon_parse_idle(mpdmon, idle);

        mpd_run_next(mpdmon->conn);

        mpd_send_idle(mpdmon->conn);
    }
}

static struct status *mpdmon_status_create(const char *server, int port, int timeout)
{
    struct mpdmon *mpdmon;

    mpdmon = malloc(sizeof(*mpdmon));
    memset(mpdmon, 0, sizeof(*mpdmon));
    status_init(&mpdmon->status);
    mpdmon->server = strdup(server);
    mpdmon->port = port;
    mpdmon->timeout = timeout;

    mpdmon->status.cmds[0].id = strdup("toggle");
    mpdmon->status.cmds[0].cmd = strdup("toggle");
    mpdmon->status.cmds[0].cmd_handle = mpdmon_handle_cmd_toggle;

    mpdmon->status.cmds[2].id = strdup("next");
    mpdmon->status.cmds[2].cmd = strdup("next");
    mpdmon->status.cmds[2].cmd_handle = mpdmon_handle_cmd_next;

    flag_set(&mpdmon->status.flags, STATUS_VISIBLE);

    mpdmon_connection_check_up(mpdmon);

    if (!mpdmon->conn) {
        flag_clear(&mpdmon->status.flags, STATUS_VISIBLE);
        g_timeout_add_seconds(timeout, mpdmon_timeout_check, mpdmon);
    } else {
        mpdmon_connection_up(mpdmon);
    }

    return &mpdmon->status;
}

static struct status *mpdmon_create(struct status_config_item *list)
{
    const char *server = status_config_get_str(list, "server");
    int port = status_config_get_int(list, "port");
    int timeout = status_config_get_int(list, "timeout");

    if (!server) {
        dbgprintf("mpd: Error, server is empty\n");
        return NULL;
    }

    return mpdmon_status_create(server, port, timeout);
}

const struct status_description mpdmon_status_description = {
    .name = "mpd",
    .items = (struct status_config_item []) {
        STATUS_CONFIG_ITEM_STR("server", "127.0.0.1"),
        STATUS_CONFIG_ITEM_INT("port", 6600),
        STATUS_CONFIG_ITEM_INT("timeout", 60),
        STATUS_CONFIG_END(),
    },
    .status_create = mpdmon_create,
};

