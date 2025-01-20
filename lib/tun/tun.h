#ifndef _TUN_H
#define _TUN_H

#ifdef __cplusplus
extern "C" {
#endif


int tun_open(char *ifname, int is_tap);
int set_tun_ip(const char* ifname, const char* ip_addr, const char* netmask);


#ifdef __cplusplus
}
#endif

#endif