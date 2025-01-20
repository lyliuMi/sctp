#include "tun.h"
#include "l_core.h"

#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_tun.h>

int tun_open(char *ifname, int is_tap)
{
    int fd = -1;
    const char *dev = "/dev/net/tun";
    int rc;
    struct ifreq ifr;
    int flags = IFF_NO_PI;

    assert(ifname);

    fd = open(dev, O_RDWR);
    if (fd < 0) {
        log_error("open() failed : dev[%s]", dev);
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = (is_tap ? (flags | IFF_TAP) : (flags | IFF_TUN));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ-1);

    rc = ioctl(fd, TUNSETIFF, (void *)&ifr);
    if (rc < 0) {
        log_error("ioctl() failed : %s dev[%s] flags[0x%x]", strerror(errno), dev, flags);
        goto cleanup;
    }

    return fd;

cleanup:
    close(fd);
    return -1;
}
int set_tun_ip(const char* ifname, const char* ip_addr, const char* netmask)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
    {
        perror("create sock fail!");
        exit(1);
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    // 配置ip
    struct sockaddr_in* addr = (struct sockaddr_in*)&ifr.ifr_addr;
    addr->sin_family = AF_INET;
    inet_pton(AF_INET, ip_addr, &addr->sin_addr);

    if(ioctl(sock, SIOCSIFADDR, &ifr) < 0)
    {
        perror("设置 IP 地址失败");
        close(sock);
        exit(1);
    }

    // 设置子网掩码
    inet_pton(AF_INET, netmask, &addr->sin_addr);
    if (ioctl(sock, SIOCSIFNETMASK, &ifr) < 0) {
        perror("设置子网掩码失败");
        close(sock);
        exit(1);
    }

    // 启用接口
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        perror("获取接口标志失败");
        close(sock);
        exit(1);
    }

    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
        perror("启用接口失败");
        close(sock);
        exit(1);
    }
    log_info("TUN dev name = %s, ip = %s, netmask = %s\n", ifname, ip_addr, netmask);
    close(sock);
    return 0;
}