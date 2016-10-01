
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

static void start_lemonbar_child(struct lemonbar *bar, const char *exe, char *const *argv)
{
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    dup2(bar->writefd[PREAD], STDIN_FILENO);
    dup2(bar->readfd[PWRITE], STDOUT_FILENO);
    dup2(bar->readfd[PWRITE], STDERR_FILENO);

    execvp(exe, argv);
}

void lemonbar_start(struct lemonbar *bar, const char *exe, char *const *argv)
{
    char *const *arg;

    pipe(bar->writefd);
    pipe(bar->readfd);

    fcntl(bar->writefd[PWRITE], F_SETFD, fcntl(bar->writefd[PWRITE], F_GETFD) | FD_CLOEXEC);
    fcntl(bar->writefd[PREAD], F_SETFD, fcntl(bar->writefd[PREAD], F_GETFD) | FD_CLOEXEC);

    fcntl(bar->readfd[PREAD], F_SETFD, fcntl(bar->readfd[PREAD], F_GETFD) | FD_CLOEXEC);
    fcntl(bar->readfd[PWRITE], F_SETFD, fcntl(bar->readfd[PWRITE], F_GETFD) | FD_CLOEXEC);

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

