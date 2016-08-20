
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

static struct mpd_connection *mpd_conn = NULL;
static struct status mpdmon_status = STATUS_INIT(mpdmon_status);

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

static void mpdmon_set_text(const char *text)
{
    if (mpdmon_status.text)
        free(mpdmon_status.text);

    mpdmon_status.text = strdup(text);
}

static void mpdmon_update_status(struct mpd_connection *conn)
{
    char buf[128];
    char song_name[128];
    struct mpd_song *song;
    struct mpd_status *status;
    const char *state;

    fprintf(stderr, "Mpd status update");

    status = mpd_run_status(conn);
    fprintf(stderr, "Status: %p\n", status);
    if (!status)
        goto cleanup_status;

    song = mpd_run_current_song(conn);

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
        mpdmon_set_text(buf);
        break;

    case MPD_STATE_UNKNOWN:
    case MPD_STATE_STOP:
        mpdmon_set_text("Mpd: Disconnected");
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
    int idle;

    idle = mpd_recv_idle(mpd_conn, false);

    fprintf(stderr, "MPD idle: %d\n", idle);

    if (idle == 0) {
        mpd_connection_free(mpd_conn);
        mpd_conn = NULL;
        g_timeout_add(MPD_TIMEOUT, mpdmon_timeout_check, NULL);

        mpdmon_set_text("Mpd: Disconnected");
        bar_state_render(&bar_state);
        return FALSE;
    }

    if ((idle & MPD_IDLE_PLAYER)) {
        mpdmon_update_status(mpd_conn);
        bar_state_render(&bar_state);
    }

    mpd_send_idle(mpd_conn);
    return TRUE;
}

static void mpdmon_connection_check_up(void)
{
    mpd_conn = mpd_connection_new(MPD_SERVER, MPD_PORT, 0);

    if (mpd_connection_get_error(mpd_conn) != MPD_ERROR_SUCCESS) {
        mpd_connection_free(mpd_conn);
        mpd_conn = NULL;
        return ;
    }
}

static void mpdmon_connection_up(void)
{
    fprintf(stderr, "mpd_connection_up\n");
    GIOChannel *gio_read;

    gio_read = g_io_channel_unix_new(mpd_connection_get_fd(mpd_conn));
    g_io_add_watch(gio_read, G_IO_IN, mpd_handle_idle, NULL);

    mpdmon_update_status(mpd_conn);
    mpd_send_idle(mpd_conn);
}

static gboolean mpdmon_timeout_check(gpointer data)
{
    mpdmon_connection_check_up();
    if (mpd_conn) {
        mpdmon_connection_up();
        bar_state_render(&bar_state);
        return FALSE;
    }

    return TRUE;
}

void mpdmon_setup(i3ipcConnection *conn)
{
    status_list_add(&bar_state.status_list, &mpdmon_status);

    flag_set(&mpdmon_status.flags, STATUS_VISIBLE);

    mpdmon_connection_check_up();

    if (!mpd_conn) {
        mpdmon_set_text("Mpd: Disconnected");
        g_timeout_add(MPD_TIMEOUT, mpdmon_timeout_check, NULL);
    } else {
        mpdmon_connection_up();
    }
}

