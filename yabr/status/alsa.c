
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <alsa/asoundlib.h>

#include "status.h"
#include "render.h"
#include "bar_config.h"

struct alsa {
    struct status status;
    struct bar_state *bar_state;

    snd_mixer_t *mixer;
    snd_mixer_elem_t *elem;
};

static long alsa_convert_to_percent(long volume, long min, long max)
{
    /* Weird math avoids floating point */
    return (volume * 100 + (max - min) - 1) / (max - min) + min;
}

static int alsa_open_mixer(struct alsa *alsa, const char *card, const char *selem)
{
    int ret;
    snd_mixer_selem_id_t *mixer_sid;

    ret = snd_mixer_open(&alsa->mixer, 0);
    if (ret < 0)
        return ret;

    ret = snd_mixer_attach(alsa->mixer, card);
    if (ret < 0)
        goto cleanup_mixer;

    ret = snd_mixer_selem_register(alsa->mixer, NULL, NULL);
    if (ret < 0)
        goto cleanup_mixer;

    ret = snd_mixer_load(alsa->mixer);
    if (ret < 0)
        goto cleanup_mixer;

    snd_mixer_selem_id_alloca(&mixer_sid);
    snd_mixer_selem_id_set_index(mixer_sid, 0);
    snd_mixer_selem_id_set_name(mixer_sid, selem);

    alsa->elem = snd_mixer_find_selem(alsa->mixer, mixer_sid);
    if (!alsa->elem) {
        ret = -1;
        goto cleanup_mixer;
    }

    return 0;

  cleanup_mixer:
    if (alsa->mixer)
        snd_mixer_close(alsa->mixer);

    return ret;
}

static long alsa_get_volume(struct alsa *alsa)
{
    long volume;
    long max, min;

    snd_mixer_selem_get_playback_volume_range(alsa->elem, &min, &max);
    snd_mixer_selem_get_playback_volume(alsa->elem, 0, &volume);

    volume = alsa_convert_to_percent(volume, min, max);

    return volume;
}

static int alsa_is_muted(struct alsa *alsa)
{
    int mut = 0;
    int ret;

    ret = snd_mixer_selem_get_playback_switch(alsa->elem, SND_MIXER_SCHN_MONO, &mut);
    if (ret)
        return 0;

    return !mut;
}

static void alsa_set_volume_status(struct alsa *alsa)
{
    char buf[128];

    if (alsa_is_muted(alsa))
        sprintf(buf, "Volume: Muted");
    else
        sprintf(buf, "Volume: %ld%%", alsa_get_volume(alsa));

    status_change_text(&alsa->status, buf);
    flag_set(&alsa->status.flags, STATUS_VISIBLE);
}

static gboolean alsa_handle_change(GIOChannel *gio, GIOCondition condition, gpointer data)
{
    struct alsa *alsa = data;

    snd_mixer_handle_events(alsa->mixer);
    alsa_set_volume_status(alsa);

    bar_state_render(alsa->bar_state);
    return TRUE;
}

static void alsa_create_cmds(struct alsa *alsa, const char *mix, const char *card)
{
    char buf[128];

    snprintf(buf, sizeof(buf), "amixer -D \"%s\" sset \"%s\" 1%%+", card, mix);
    alsa->status.cmds[3].cmd = strdup(buf);

    snprintf(buf, sizeof(buf), "amixer -D \"%s\" sset \"%s\" 1%%-", card, mix);
    alsa->status.cmds[4].cmd = strdup(buf);
}

void alsa_status_add(struct bar_state *state, const char *mix, const char *card)
{
    struct alsa *alsa;
    GIOChannel *gio_read;
    struct pollfd *poll_array;
    size_t poll_size;
    int ret, i;

    alsa = malloc(sizeof(*alsa));
    status_init(&alsa->status);
    alsa->bar_state = state;

    ret = alsa_open_mixer(alsa, card, mix);
    if (ret)
        return ;

    alsa_set_volume_status(alsa);

    poll_size = sizeof(*poll_array) * snd_mixer_poll_descriptors_count(alsa->mixer);
    poll_array = alloca(poll_size);

    snd_mixer_poll_descriptors(alsa->mixer, poll_array, poll_size);

    for (i = 0; i < poll_size / sizeof(*poll_array); i++) {
        gio_read = g_io_channel_unix_new(poll_array[i].fd);
        g_io_add_watch(gio_read, G_IO_IN, alsa_handle_change, alsa);
    }

    alsa_create_cmds(alsa, mix, card);

    status_list_add(&state->status_list, &alsa->status);

    return ;
}

