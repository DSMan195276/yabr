
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/inotify.h>

#include <glib/gprintf.h>

#include "render.h"
#include "bar_config.h"
#include "config.h"
#include "status_desc.h"
#include "mail.h"

struct mail {
    struct status status;

    char *name;
    char *mail_dir;
    int new_count;

    int inotifyfd;

    int urgent_timeout;
    int urgent_count;
};

#define INOTIFY_BUF_LEN (128 * (sizeof(struct inotify_event) + 16))

static int count_new_mail(const char *mail_dir)
{
    char path[256];
    DIR *dir;
    int count = 0;
    struct dirent *ent;

    snprintf(path, sizeof(path), "%s/new", mail_dir);

    dir = opendir(path);
    if (!dir) {
        dbgprintf("Unable to open mail dir: %s\n", path);
        return 0;
    }

    while ((ent = readdir(dir)) != NULL)
        count++;

    closedir(dir);

    count -= 2;

    if (count < 0)
        count = 0;

    dbgprintf("New mail %s count: %d\n", mail_dir, count);

    return count;
}

static gboolean mail_end_urgent(gpointer data)
{
    struct mail *mail = data;

    mail->urgent_count--;

    if (mail->urgent_count == 0) {
        flag_clear(&mail->status.flags, STATUS_URGENT);
        bar_render_global();
    }

    return FALSE;
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
        flag_clear(&mail->status.flags, STATUS_URGENT);
        return 1;
    }

    snprintf(buf, sizeof(buf), "%s: %d new", mail->name, mail->new_count);

    status_change_text(&mail->status, buf);
    flag_set(&mail->status.flags, STATUS_VISIBLE);

    if (mail->new_count > old_count && mail->urgent_timeout > 0) {
        mail->urgent_count++;
        flag_set(&mail->status.flags, STATUS_URGENT);
        g_timeout_add_seconds(mail->urgent_timeout, mail_end_urgent, mail);
    }

    return 1;
}

static gboolean mail_check_inotify(GIOChannel *gio, GIOCondition condition, gpointer data)
{
    char buf[INOTIFY_BUF_LEN];
    int len;
    struct inotify_event *event;
    struct mail *mail = data;

    len = read(mail->inotifyfd, buf, sizeof(buf));

    /* 
     * Note: Code below processes through each inotify event, but since we're
     * only watching one file we don't actually care about the events. the
     * `read` call is still necessary to clear the buffer however.
     */
    for (event = (struct inotify_event *)buf;
         (char *)event < buf + len;
         event = (struct inotify_event *)((char *)event + sizeof(*event) + event->len)) {
        if (event->len)
            dbgprintf("mail: Event name: %s\n", event->name);
    }

    if (mail_update(mail))
        bar_render_global();

    return TRUE;
}

static void mail_handle_cmd_ack(struct status *status, struct cmd_entry *cmd, const char *args)
{
    struct mail *mail = container_of(status, struct mail, status);

    dbgprintf("mail: New mail ack\n");

    if (mail->urgent_timeout > 0) {
        flag_clear(&mail->status.flags, STATUS_URGENT);
        bar_render_global();
    }
}

static struct status *mail_status_create(const char *name, const char *mail_dir, int timeout)
{
    struct mail *mail;
    GIOChannel *gio_read;

    mail = malloc(sizeof(*mail));
    memset(mail, 0, sizeof(*mail));
    status_init(&mail->status);
    mail->name = strdup(name);
    mail->mail_dir = strdup(mail_dir);
    mail->inotifyfd = inotify_init();
    mail->urgent_timeout = timeout;

    mail->status.cmds[0].id = strdup("ack");
    mail->status.cmds[0].cmd = strdup("ack");
    mail->status.cmds[0].cmd_handle = mail_handle_cmd_ack;

    mail_update(mail);

    char dir[PATH_MAX];
    snprintf(dir, sizeof(dir), "%s/new", mail_dir);

    inotify_add_watch(mail->inotifyfd, dir, IN_CREATE | IN_MOVED_TO | IN_MOVED_FROM | IN_MODIFY | IN_DELETE);

    fcntl(mail->inotifyfd, F_SETFD, fcntl(mail->inotifyfd, F_GETFD) | FD_CLOEXEC);

    gio_read = g_io_channel_unix_new(mail->inotifyfd);
    g_io_add_watch(gio_read, G_IO_IN, mail_check_inotify, mail);

    return &mail->status;
}

static struct status *mail_create(struct status_config_item *list)
{
    const char *name = status_config_get_str(list, "name");
    const char *maildir = status_config_get_str(list, "maildir");
    int timeout = status_config_get_int(list, "urgent_timeout");

    if (!name) {
        dbgprintf("mail: Error, name is empty\n");
        return NULL;
    }

    if (!maildir) {
        dbgprintf("mail: Error, maildir is empty\n");
        return NULL;
    }

    return mail_status_create(name, maildir, timeout);
}

const struct status_description mail_status_description = {
    .name = "mail",
    .items = (struct status_config_item []) {
        STATUS_CONFIG_ITEM_STR("name", "Gmail"),
        STATUS_CONFIG_ITEM_STR("maildir", "/mnt/data/mail/Inbox"),
        STATUS_CONFIG_ITEM_INT("urgent_timeout", 60),
        STATUS_CONFIG_END(),
    },
    .status_create = mail_create,
};

