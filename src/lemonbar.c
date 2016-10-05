
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#include "config.h"
#include "lemonbar.h"

#define PREAD 0
#define PWRITE 1

/* 
 * Close all file descriptors above a certain number.
 *
 * While in theory we don't need this with careful handling of close-on-exec,
 * in practice it's simply not possible to avoid this. Some of the libraries we
 * use (i3ipc-glib in paticular) create file-descriptors without close-on-exec
 * and provide no access to the underlying fds, making it impossible to mark
 * them as close-on-exec.
 */
static void close_all_fds_above(int max)
{
    DIR *d;
    struct dirent *ent;

    /* This is a very Linux-specefic solution - loop over the contents of
     * /proc/self/fd and the fds. */
    d = opendir("/proc/self/fd/");

    while ((ent = readdir(d)) != NULL) {
        long fd;

        if (ent->d_name[0] == '.')
            continue;

        fd = strtol(ent->d_name, NULL, 10);

        if (fd <= max ||fd == dirfd(d))
            continue;

        close(fd);
    }

    closedir(d);
}

static void start_lemonbar_child(struct lemonbar *bar, const char *exe, char *const *argv)
{
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    dup2(bar->writefd[PREAD], STDIN_FILENO);
    dup2(bar->readfd[PWRITE], STDOUT_FILENO);
    dup2(bar->readfd[PWRITE], STDERR_FILENO);

    close_all_fds_above(STDERR_FILENO);

    execvp(exe, argv);
}

void lemonbar_start(struct lemonbar *bar, const char *exe, char *const *argv)
{
    char *const *arg;

    pipe2(bar->writefd, O_CLOEXEC);
    pipe2(bar->readfd, O_CLOEXEC);

    dbgprintf("Lemonbar exec: %s", exe);
    for (arg = argv; *arg; arg++)
        dbgprintf(" %s", *arg);
    dbgprintf("\n");

    switch ((bar->pid = fork())) {
    case 0:
        start_lemonbar_child(bar, exe, argv);
        exit(1);

    case -1:
        dbgprintf("Error: Unable to fork lemonbar.\n");
        break;

    default:
        break;
    }

    dbgprintf("Lemonbar pid: %d\n", bar->pid);

    bar->read_file = fdopen(bar->readfd[PREAD], "r");
    bar->write_file = fdopen(bar->writefd[PWRITE], "w");
}

void lemonbar_kill(struct lemonbar *bar)
{
    fclose(bar->read_file);
    fclose(bar->write_file);

    kill(bar->pid, SIGKILL);
    waitpid(bar->pid, NULL, 0);
}

