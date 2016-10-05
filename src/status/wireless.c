
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
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
#include "config.h"
#include "status_desc.h"
#include "bar_config.h"

enum wireless_state {
    NO_IFACE,
    NO_IP,
    HAS_IP,
};

struct wireless {
    struct status status;

    char *iface;

    int netlink_socket;
    enum wireless_state state;
    char essid[IFNAMSIZ];
    uint32_t ip;
    int if_index;
};

static void wireless_render_status(struct wireless *wireless)
{
    char buf[128];
    struct in_addr addr;

    switch (wireless->state) {
    case HAS_IP:
        addr.s_addr = htonl(wireless->ip);

        if (!*wireless->essid) {
            snprintf(buf, sizeof(buf), "%s: %s", wireless->iface, inet_ntoa(addr));
        } else {
            snprintf(buf, sizeof(buf), "%s: %s", wireless->iface, wireless->essid);
        }
        dbgprintf("wireless: Got IP: %s\n", buf);
        goto setup_status;

    case NO_IP:
        snprintf(buf, sizeof(buf), "%s: Disconnected", wireless->iface);
        dbgprintf("wireless: Disconnected.\n");
        goto setup_status;

    setup_status:
        status_change_text(&wireless->status, buf);
        flag_set(&wireless->status.flags, STATUS_VISIBLE);
        break;

    case NO_IFACE:
        flag_clear(&wireless->status.flags, STATUS_VISIBLE);
        break;
    }
}

static int wireless_get_settings(struct wireless *wireless)
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
    wireless->state = NO_IFACE;

    for (cur_addr = addrs; cur_addr; cur_addr = cur_addr->ifa_next) {
        if (strcmp(cur_addr->ifa_name, wireless->iface) == 0) {
            struct sockaddr_in *sa;

            if (cur_addr->ifa_addr->sa_family != AF_INET) {
                if (wireless->state == NO_IFACE)
                    wireless->state = NO_IP;
                continue;
            }

            sa = (struct sockaddr_in *)cur_addr->ifa_addr;

            wireless->ip = ntohl(sa->sin_addr.s_addr);
            wireless->state = HAS_IP;

            memset(&req, 0, sizeof(req));
            strcpy(req.ifr_name, wireless->iface);
            req.u.essid.pointer = wireless->essid;
            req.u.essid.length = sizeof(wireless->essid);

            if (ioctl(wireless->netlink_socket, SIOCGIWESSID, &req) < 0)
                wireless->essid[0] = '\0';
            else
                wireless->essid[req.u.essid.length] = '\0';

            break;
        }
    }

    freeifaddrs(addrs);

    return 0;
}

static int wireless_open_netlink(struct wireless *wireless)
{
    struct sockaddr_nl addr;

    wireless->netlink_socket = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (wireless->netlink_socket < 0)
        return -1;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_IPV4_IFADDR;

    if (bind(wireless->netlink_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(wireless->netlink_socket);
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
    struct wireless *wireless = data;
    int len;
    struct nlmsghdr *nlh;
    char buffer[4096];
    int change = 0;

    len = recv(wireless->netlink_socket, buffer, sizeof(buffer), 0);

    if (!len)
        return TRUE;

    nlh = (struct nlmsghdr *)buffer;

    for (; NLMSG_OK(nlh, len) && (nlh->nlmsg_type != NLMSG_DONE); nlh = NLMSG_NEXT(nlh, len)) {
        if (nlh->nlmsg_type == RTM_NEWADDR || nlh->nlmsg_type == RTM_DELADDR) {
            struct ifaddrmsg *ifa = (struct ifaddrmsg *)NLMSG_DATA(nlh);

            if (ifa->ifa_index == wireless->if_index) {
                wireless_get_settings(wireless);
                change = 1;
            }
        }
    }

    if (change) {
        wireless_render_status(wireless);
        bar_render_global();
    }

    return TRUE;
}

static struct status *wireless_status_create(const char *ifname)
{
    struct wireless *wireless;
    GIOChannel *gio_read;
    int ret;

    wireless = malloc(sizeof(*wireless));
    memset(wireless, 0, sizeof(*wireless));
    status_init(&wireless->status);

    wireless->if_index = if_nametoindex(ifname);
    if (!wireless->if_index)
        goto cleanup_wireless;

    wireless->iface = strdup(ifname);

    ret = wireless_open_netlink(wireless);
    if (ret)
        goto cleanup_wireless;

    ret = wireless_get_settings(wireless);
    if (ret)
        goto cleanup_wireless;

    wireless_render_status(wireless);

    fcntl(wireless->netlink_socket, F_SETFD, fcntl(wireless->netlink_socket, F_GETFD) | FD_CLOEXEC);

    gio_read = g_io_channel_unix_new(wireless->netlink_socket);
    g_io_add_watch(gio_read, G_IO_IN, wireless_handle_netlink, wireless);

    return &wireless->status;

  cleanup_wireless:
    if (wireless->netlink_socket)
        close(wireless->netlink_socket);

    if (wireless->iface)
        free(wireless->iface);

    free(wireless);
    return NULL;
}

static struct status *wireless_create(struct status_config_item *list)
{
    const char *iface = status_config_get_str(list, "iface");

    if (!iface) {
        dbgprintf("wlreless: Error, iface is empty\n");
        return NULL;
    }

    return wireless_status_create(iface);
}

const struct status_description wireless_status_description = {
    .name = "wireless",
    .items = (struct status_config_item []) {
        STATUS_CONFIG_ITEM_STR("iface", "wlp3s0"),
        STATUS_CONFIG_END(),
    },
    .status_create = wireless_create,
};

