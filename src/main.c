
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include <glib/gprintf.h>

#include "bar_config.h"
#include "ws.h"
#include "render.h"
#include "outputs.h"
#include "status.h"
#include "config.h"
#include "notif_dbus.h"
#include "yabr_parser.h"

void bar_render_global(void)
{
    bar_state_render(&yabr_config.state);
}

static void usage(const char *prog)
{
    printf("%s: [-vh] [-c config-file] [-l log-file]\n", prog);
    printf("\n");
    printf(" -v : Show version information\n");
    printf(" -h : Show this help\n");
    printf(" -c : Use 'config-file' instead of default ~/.yabrrc\n");
    printf(" -l : Log to 'log-file' instead of stderr\n");
}

static void version(const char *prog)
{
    printf("%s: Yet Another Bar Replacement\n", prog);
    printf("  Version: " Q(YABR_VERSION_N) "\n");
    printf("  Written by Matthew Kilgore\n");
}

int main(int argc, char **argv)
{
    char cfg_file[256];
    int ret;

    if (getenv("HOME"))
        snprintf(cfg_file, sizeof(cfg_file), "%s/.yabrrc", getenv("HOME"));
    else
        snprintf(cfg_file, sizeof(cfg_file), "/etc/yabrrc");

    yabr_config.config_file = cfg_file;

    while ((ret = getopt(argc, argv, "hvc:l:")) != -1) {
        switch (ret) {
        case 'c':
            yabr_config.config_file = optarg;
            break;

        case 'h':
            usage(argv[0]);
            exit(0);

        case 'v':
            version(argv[0]);
            exit(0);

        case 'l':
            if (yabr_config.debug) {
                fprintf(stderr, "%s: Please only supply one log file.\n", argv[0]);
                exit(1);
            }
            yabr_config.debug = fopen(optarg, "a");
            if (!yabr_config.debug) {
                perror(optarg);
                exit(1);
            }
            break;

        case '?':
            usage(argv[0]);
            exit(1);
        }
    }

    if (!yabr_config.debug)
        yabr_config.debug = stderr;

    if (optind < argc) {
        fprintf(stderr, "%s: Unused argument \"%s\"\n", argv[0], argv[optind]);
        exit(1);
    }

    ret = load_config_file();

    if (ret)
        return 1;

    i3_state_setup(&yabr_config.state);

    if (yabr_config.use_notifications) {
        ret = notif_dbus_open(&yabr_config.state.notif);
        if (ret)
            dbgprintf("Unable to open notification daemon\n");
    }

    outputs_update(&yabr_config.state.i3_state, &yabr_config.state);

    ws_list_refresh(&yabr_config.state.ws_list, &yabr_config.state.i3_state);
    i3_state_update_title(&yabr_config.state);
    bar_state_render(&yabr_config.state);

    i3_state_main(&yabr_config.state);

    i3_state_close(&yabr_config.state);

    ws_list_clear(&yabr_config.state.ws_list);

    return 0;
}

