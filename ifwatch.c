/**
 * @file ifwatch.c
 * @author Stefano Moioli (smxdev4@gmail.com)
 * @brief Watches for IP changes on any interface
 * @version 0.1
 * @date 2023-06-27
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <arpa/inet.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define PRINTF_LEN(maxlen) "%." STR(maxlen) "s"

void report_op(const char *module, const char *op, const char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);

    fprintf(stdout, "%s:%s:", module, op);
    vprintf(fmt, ap);
    
    va_end(ap);

    
    fputc('\n', stdout);
}

void on_rtm_addr(unsigned short op, struct nlmsghdr *nlh){
    struct ifaddrmsg *ifa = (struct ifaddrmsg *) NLMSG_DATA(nlh);
    if(ifa == NULL){
        return;
    }

    struct rtattr *rth = IFA_RTA(ifa);
    int rtl = IFA_PAYLOAD(nlh);

    char name[IFNAMSIZ];
    memset(name, 0x00, sizeof(name));

    for(;
        rtl > 0 && RTA_OK(rth, rth);
        rth = RTA_NEXT(rth, rtl)
    ){
        if (rth->rta_type != IFA_LOCAL) {
            continue;
        }

        if_indextoname(ifa->ifa_index, name);

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, RTA_DATA(rth), ip, sizeof(ip));

        report_op(
            "ip",
            (op == RTM_NEWADDR) ? "add" : "del",
            PRINTF_LEN(IFNAMSIZ) ":" PRINTF_LEN(INET_ADDRSTRLEN),
            name, ip
        );
    }
}

void on_rtm_link(unsigned short op, struct nlmsghdr *nlh){
    struct ifinfomsg *ifi = (struct ifinfomsg *) NLMSG_DATA(nlh);
    if(ifi == NULL){
        return;
    }

    struct rtattr *rth = IFA_RTA(ifi);
    int rtl = IFA_PAYLOAD(nlh);

    char name[IFNAMSIZ];
    memset(name, 0x00, sizeof(name));

    for(;
        rtl > 0 && RTA_OK(rth, rth);
        rth = RTA_NEXT(rth, rtl)
    ){
        if_indextoname(ifi->ifi_index, name);

        report_op(
            "link",
            (ifi->ifi_flags & IFF_UP) ? "up" : "down",
            PRINTF_LEN(IFNAMSIZ), name
        );
    }
}

void on_rtm_route(unsigned short op, struct nlmsghdr *nlh){
    struct rtmsg *route_entry = (struct rtmsg *) NLMSG_DATA(nlh);
    if(route_entry == NULL){
        return;
    }

    struct rtattr *route_attribute = (struct rtattr *) RTM_RTA(route_entry);
    int rtl = RTM_PAYLOAD(nlh);

    char dst_addr[INET_ADDRSTRLEN];
    char gw_addr[INET_ADDRSTRLEN];
    memset(dst_addr, 0x00, sizeof(dst_addr));
    memset(gw_addr, 0x00, sizeof(dst_addr));

    for (;
        rtl > 0 && RTA_OK(route_attribute, rtl);
        route_attribute = RTA_NEXT(route_attribute, rtl)
    ){
        switch(route_attribute->rta_type){
            case RTA_DST:
                inet_ntop(AF_INET, RTA_DATA(route_attribute), dst_addr, sizeof(dst_addr));
                break;
            case RTA_GATEWAY:
                inet_ntop(AF_INET, RTA_DATA(route_attribute), gw_addr, sizeof(dst_addr));
                break;
        }
    }

    report_op(
        "route",
        (op == RTM_NEWROUTE) ? "add" : "del",
        PRINTF_LEN(INET_ADDRSTRLEN) ":" PRINTF_LEN(INET_ADDRSTRLEN),
        dst_addr, gw_addr
    );
}

int main() {
    struct sockaddr_nl addr;
    int sock, len;
    unsigned char buffer[4096];

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    if ((sock = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) == -1) {
        perror("couldn't open NETLINK_ROUTE socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE | RTMGRP_LINK;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("couldn't bind");
        return 1;
    }

    memset(&buffer, 0x00, sizeof(buffer));
    for(len = 0;
        (len = recv(sock, buffer, sizeof(buffer), 0)) > 0;
    ){
        for(
            struct nlmsghdr *nlh = (struct nlmsghdr *)&buffer;
            NLMSG_OK(nlh, len) && nlh->nlmsg_type != NLMSG_DONE;
            nlh = NLMSG_NEXT(nlh, len)
        ){
            switch(nlh->nlmsg_type){
                case RTM_NEWADDR:
                case RTM_DELADDR:
                    on_rtm_addr(nlh->nlmsg_type, nlh);
                    break;
                case RTM_NEWROUTE:
                case RTM_DELROUTE:
                    on_rtm_route(nlh->nlmsg_type, nlh);
                    break;
                case RTM_NEWLINK:
                case RTM_DELLINK:
                    on_rtm_link(nlh->nlmsg_type, nlh);
                    break;
            }
        }
    }
    return 0;
}
