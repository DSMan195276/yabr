
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

/* Note: Some things start as 'modmon' instead of 'mpd' to avoid name conflicts */

struct mpdmon {
    struct status status;
    struct mpd_connection *conn;
    struct bar_state *bar_state;

    const char *server;
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

    fprintf(stderr, "Mpd status update");

    status = mpd_run_status(mpdmon->conn);
    fprintf(stderr, "Status: %p\n", status);
    if (!status)
        goto cleanup_status;

    song = mpd_run_current_song(mpdmon->conn);

    fprintf(stderr, "State: %d\n", mpd_status_get_state(status));

    switch (mpd_status_get_state(status)) {
    case MPD_STATE_PLAY:
        state = "Playing";
        goto make_title;

    case MPD_STATE_PAUSE:
        state = "Paused";
        goto make_title;

    make_title:
        song_to_string(song, song_name, sizeof(song_name));
        snprintf(buf, sizeof(buf), "Mpd: %s - %s",
                state,
                song_name);
        status_change_text(&mpdmon->status, buf);
        break;

    case MPD_STATE_UNKNOWN:
    case MPD_STATE_STOP:
        status_change_text(&mpdmon->status, "Mpd: Disconnected");
        break;
    }

    if (song)
        mpd_song_free(song);

  cleanup_status:
    mpd_status_free(status);
    return ;
}

static gboolean mpdmon_timeout_check(gpointer data);

static gboolean mpd_handle_idle(GIOChannel *gio, GIOCondition condition, gpointer data)
{
    struct mpdmon *mpdmon = data;
    int idle;

    idle = mpd_recv_idle(mpdmon->conn, false);

    fprintf(stderr, "MPD idle: %d\n", idle);

    if (idle == 0) {
        mpd_connection_free(mpdmon->conn);
        mpdmon->conn = NULL;
        g_timeout_add_seconds(mpdmon->timeout, mpdmon_timeout_check, mpdmon);

        status_change_text(&mpdmon->status, "Mpd: Disconnected");
        bar_state_render(mpdmon->bar_state);
        return FALSE;
    }

    if ((idle & MPD_IDLE_PLAYER)) {
        mpdmon_update_status(mpdmon);
        bar_state_render(mpdmon->bar_state);
    }

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
        bar_state_render(mpdmon->bar_state);
        return FALSE;
    }

    return TRUE;
}

void mpdmon_status_add(struct bar_state *state, const char *server, int port, int timeout)
{
    struct mpdmon *mpdmon;

    mpdmon = malloc(sizeof(*mpdmon));
    memset(mpdmon, 0, sizeof(*mpdmon));
    status_init(&mpdmon->status);
    mpdmon->bar_state = state;
    mpdmon->server = server;
    mpdmon->port = port;
    mpdmon->timeout = timeout;

    flag_set(&mpdmon->status.flags, STATUS_VISIBLE);

    mpdmon_connection_check_up(mpdmon);

    if (!mpdmon->conn) {
        status_change_text(&mpdmon->status, "Mpd: Disconnected");
        g_timeout_add_seconds(timeout, mpdmon_timeout_check, mpdmon);
    } else {
        mpdmon_connection_up(mpdmon);
    }

    status_list_add(&state->status_list, &mpdmon->status);
}

