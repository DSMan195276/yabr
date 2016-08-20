
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <alsa/asoundlib.h>

#include "status.h"
#include "render.h"
#include "bar_config.h"

struct status alsa_status = STATUS_INIT(alsa_status);

static snd_mixer_t *cur_mixer;
static snd_mixer_elem_t *cur_elem;

static long alsa_convert_to_percent(long volume, long min, long max)
{
    /* Weird math avoids floating point */
    return (volume * 100 + (max - min) - 1) / (max - min) + min;
}

static int alsa_open_mixer(const char *card, const char *selem)
{
    int ret;
    snd_mixer_selem_id_t *mixer_sid;

    ret = snd_mixer_open(&cur_mixer, 0);
    if (ret < 0)
        return ret;

    ret = snd_mixer_attach(cur_mixer, card);
    if (ret < 0)
        goto cleanup_mixer;

    ret = snd_mixer_selem_register(cur_mixer, NULL, NULL);
    if (ret < 0)
        goto cleanup_mixer;

    ret = snd_mixer_load(cur_mixer);
    if (ret < 0)
        goto cleanup_mixer;

    snd_mixer_selem_id_alloca(&mixer_sid);
    snd_mixer_selem_id_set_index(mixer_sid, 0);
    snd_mixer_selem_id_set_name(mixer_sid, selem);

    cur_elem = snd_mixer_find_selem(cur_mixer, mixer_sid);
    if (!cur_elem) {
        ret = -1;
        goto cleanup_mixer;
    }

    return 0;

  cleanup_mixer:
    if (cur_mixer)
        snd_mixer_close(cur_mixer);

    return ret;
}

static int alsa_close_mixer(void)
{
    if (cur_mixer)
        snd_mixer_close(cur_mixer);

    return 0;
}

static long alsa_get_volume(void)
{
    long volume;
    long max, min;

    snd_mixer_selem_get_playback_volume_range(cur_elem, &min, &max);
    snd_mixer_selem_get_playback_volume(cur_elem, 0, &volume);

    volume = alsa_convert_to_percent(volume, min, max);

    return volume;
}

static int alsa_is_muted(void)
{
    int mut = 0;
    int ret;

    ret = snd_mixer_selem_get_playback_switch(cur_elem, SND_MIXER_SCHN_MONO, &mut);
    if (ret)
        return 0;

    return !mut;
}

static void alsa_set_volume_status(void)
{
    char buf[128];

    if (alsa_is_muted())
        sprintf(buf, "Volume: Muted");
    else
        sprintf(buf, "Volume: %ld%%", alsa_get_volume());

    if (alsa_status.text)
        free(alsa_status.text);

    alsa_status.text = strdup(buf);
    flag_set(&alsa_status.flags, STATUS_VISIBLE);
}

static gboolean gio_input(GIOChannel *gio, GIOCondition condition, gpointer data)
{
    snd_mixer_handle_events(cur_mixer);
    alsa_set_volume_status();

    bar_state_render(&bar_state);
    return TRUE;
}

void alsa_setup(i3ipcConnection *conn)
{
    GIOChannel *gio_read;
    struct pollfd *poll_array;
    size_t poll_size;
    int ret;
    int i;

    status_list_add(&bar_state.status_list, &alsa_status);

    ret = alsa_open_mixer(ALSA_CARD, ALSA_MIX);
    if (ret)
        return ;

    alsa_set_volume_status();

    poll_size = sizeof(*poll_array) * snd_mixer_poll_descriptors_count(cur_mixer);
    poll_array = alloca(poll_size);

    snd_mixer_poll_descriptors(cur_mixer, poll_array, poll_size);

    for (i = 0; i < poll_size / sizeof(*poll_array); i++) {
        gio_read = g_io_channel_unix_new(poll_array[i].fd);
        g_io_add_watch(gio_read, G_IO_IN, gio_input, NULL);
    }
}

