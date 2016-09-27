
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/inotify.h>

#include <glib/gprintf.h>

#include "render.h"
#include "bar_config.h"
#include "mail.h"

struct mail {
    struct status status;

    const char *name;
    const char *mail_dir;
    int new_count;

    int inotifyfd;
};

#define INOTIFY_BUF_LEN (128 * (sizeof(struct inotify_event) + 16))

static int count_new_mail(const char *mail_dir)
{
    char exe[256];
    FILE *m;
    int count;

    snprintf(exe, sizeof(exe), "mail -H -f %s | grep \"^ N\" | wc -l", mail_dir);

    m = popen(exe, "r");

    fscanf(m, "%d", &count);
    fprintf(stderr, "New mail %s count: %d\n", mail_dir, count);

    pclose(m);

    return count;
}

static int mail_update(struct mail *mail)
{
    char buf[128];
    int old_count = mail->new_count;

    mail->new_count = count_new_mail(mail->mail_dir);

    if (mail->new_count == old_count)
        return 0;

    if (mail->new_count <= 0) {
        flag_clear(&mail->status.flags, STATUS_VISIBLE);
        return 1;
    }

    snprintf(buf, sizeof(buf), "%s: %d new", mail->name, mail->new_count);

    status_change_text(&mail->status, buf);
    flag_set(&mail->status.flags, STATUS_VISIBLE);

    return 1;
}

static gboolean mail_check_inotify(GIOChannel *gio, GIOCondition condition, gpointer data)
{
    char buf[INOTIFY_BUF_LEN];
    int len;
    struct inotify_event *event;
    struct mail *mail = data;

    len = read(mail->inotifyfd, buf, sizeof(buf));

    for (event = (struct inotify_event *)buf;
         (char *)event < buf + len;
         event = (struct inotify_event *)((char *)event + sizeof(*event) + event->len)) {
        if (event->len)
            fprintf(stderr, "Event name: %s\n", event->name);
    }

    if (mail_update(mail))
        bar_render_global();

    return TRUE;
}

static gboolean mail_check(gpointer data)
{
    struct mail *mail = data;

    if (mail_update(mail))
        bar_render_global();

    return TRUE;
}

struct status *mail_status_create(const char *name, const char *mail_dir, int timeout)
{
    struct mail *mail;
    GIOChannel *gio_read;

    mail = malloc(sizeof(*mail));
    memset(mail, 0, sizeof(*mail));
    status_init(&mail->status);
    mail->name = name;
    mail->mail_dir = mail_dir;
    mail->inotifyfd = inotify_init();

    mail_update(mail);

    /* Fallback to poll if inotify doesn't work */
    if (mail->inotifyfd < 0) {
        g_timeout_add_seconds(timeout, mail_check, mail);
    } else {
        char dir[PATH_MAX];
        snprintf(dir, sizeof(dir), "%s/new", mail_dir);

        fprintf(stderr, "inotify on %s\n", dir);

        inotify_add_watch(mail->inotifyfd, dir, IN_CREATE | IN_MOVED_TO | IN_MOVED_FROM | IN_MODIFY);

        gio_read = g_io_channel_unix_new(mail->inotifyfd);
        g_io_add_watch(gio_read, G_IO_IN, mail_check_inotify, mail);
    }

    return &mail->status;
}

