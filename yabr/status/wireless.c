
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/wireless.h>

#include <glib/gprintf.h>

#include "status.h"
#include "render.h"
#include "bar_config.h"

static struct status wireless_status = STATUS_INIT(wireless_status);

enum wireless_state {
    NO_IFACE,
    NO_IP,
    HAS_IP,
};

static int netlink_socket;
static enum wireless_state wireless_state;
static char wireless_essid[IFNAMSIZ];
static uint32_t wireless_ip;
static int wireless_if_index;

static void wireless_render_status(void)
{
    char buf[128];
    struct in_addr addr;

    switch (wireless_state) {
    case HAS_IP:
        addr.s_addr = htonl(wireless_ip);

        if (!*wireless_essid) {
            snprintf(buf, sizeof(buf), WIRELESS_IFACE ": %s", inet_ntoa(addr));
        } else {
            snprintf(buf, sizeof(buf), WIRELESS_IFACE ": %s - %s", wireless_essid, inet_ntoa(addr));
        }
        goto setup_status;

    case NO_IP:
        strcpy(buf, WIRELESS_IFACE ": Disconnected");
        goto setup_status;

    setup_status:
        if (wireless_status.text)
            free(wireless_status.text);

        wireless_status.text = strdup(buf);
        flag_set(&wireless_status.flags,STATUS_VISIBLE);
        break;
    case NO_IFACE:
        flag_clear(&wireless_status.flags, STATUS_VISIBLE);
        break;
    }
}

static int wireless_get_settings(void)
{
    struct iwreq req;
    struct ifaddrs *addrs, *cur_addr;
    int ret;

    ret = getifaddrs(&addrs);
    if (ret)
        return ret;

    /*
     * ifaces can be in any order, and there are more then one entry in this
     * list per-iface.
     *
     * Every existing iface reports an AF_PACKET entry, so every existing iface
     * will always be listed. If we never find our iface in the list, then it
     * doesn't exist.
     */
    wireless_state = NO_IFACE;

    for (cur_addr = addrs; cur_addr; cur_addr = cur_addr->ifa_next) {
        if (strcmp(cur_addr->ifa_name, WIRELESS_IFACE) == 0) {
            struct sockaddr_in *sa;

            if (cur_addr->ifa_addr->sa_family != AF_INET && wireless_state == NO_IFACE) {
                wireless_state = NO_IP;
                continue;
            }

            sa = (struct sockaddr_in *)cur_addr->ifa_addr;

            fprintf(stderr, "Family: %d\n", cur_addr->ifa_addr->sa_family);

            wireless_ip = ntohl(sa->sin_addr.s_addr);
            wireless_state = HAS_IP;

            fprintf(stderr, "%s has ip 0x%08x: %s\n", cur_addr->ifa_name, wireless_ip, inet_ntoa(sa->sin_addr));

            memset(&req, 0, sizeof(req));
            strcpy(req.ifr_name, WIRELESS_IFACE);
            req.u.essid.pointer = wireless_essid;
            req.u.essid.length = sizeof(wireless_essid);

            if (ioctl(netlink_socket, SIOCGIWESSID, &req) < 0)
                wireless_essid[0] = '\0';

            break;
        }
    }

    freeifaddrs(addrs);

    return 0;
}

static int wireless_open_netlink(void)
{
    struct sockaddr_nl addr;
    netlink_socket = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (netlink_socket < 0)
        return -1;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_IPV4_IFADDR;

    if (bind(netlink_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(netlink_socket);
        return -1;
    }

    return 0;
}

/*
 * We do little actual netlink msg parsing - We check all the messages for address
 * changes, and then check if they happened to our wireless iface. If they did,
 * then we just run wireless_get_settings() to gather all the info again.
 *
 * This could probably be slightly improved by updating the information based
 * off of what we get from netlink instead, but this will happen so little it
 * is probably not worth it - we really only use the netlink handle to get some
 * type of notification about iface changes.
 */
static gboolean wireless_handle_netlink(GIOChannel *gio, GIOCondition condition, gpointer data)
{
    int len;
    struct nlmsghdr *nlh;
    char buffer[4096];
    int change = 0;

    len = recv(netlink_socket, buffer, sizeof(buffer), 0);

    if (!len)
        return TRUE;

    nlh = (struct nlmsghdr *)buffer;

    for (;NLMSG_OK(nlh, len) && (nlh->nlmsg_type != NLMSG_DONE); nlh = NLMSG_NEXT(nlh, len)) {
        if (nlh->nlmsg_type == RTM_NEWADDR || nlh->nlmsg_type == RTM_DELADDR) {
            struct ifaddrmsg *ifa = (struct ifaddrmsg *)NLMSG_DATA(nlh);

            if (ifa->ifa_index == wireless_if_index) {
                wireless_get_settings();
                change = 1;
            }
        }
    }

    if (change) {
        wireless_render_status();
        bar_state_render(&bar_state);
    }

    return TRUE;
}

void wireless_setup(i3ipcConnection *conn)
{
    GIOChannel *gio_read;
    int ret;

    status_list_add(&bar_state.status_list, &wireless_status);

    wireless_if_index = if_nametoindex(WIRELESS_IFACE);
    if (!wireless_if_index)
        return ;

    ret = wireless_open_netlink();
    if (ret)
        return ;

    ret = wireless_get_settings();
    if (ret)
        return ;

    wireless_render_status();

    gio_read = g_io_channel_unix_new(netlink_socket);
    g_io_add_watch(gio_read, G_IO_IN, wireless_handle_netlink, NULL);

    return ;
}

