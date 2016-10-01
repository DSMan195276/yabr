#ifndef INCLUDE_LEMONBAR_H
#define INCLUDE_LEMONBAR_H

#include <stdio.h>
#include <unistd.h>

struct lemonbar {
    int writefd[2];
    int readfd[2];

    FILE *read_file, *write_file;

    pid_t pid;
};

void lemonbar_start(struct lemonbar *, const char *exe, char *const *argv);
void lemonbar_kill(struct lemonbar *);

#endif
