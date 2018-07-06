#ifndef _MY_ETHER_H
#define _MY_ETHER_H

#define ETH_ADD_BCAST "\xFF\xFF\xFF\xFF\xFF\xFF"

#define MAC_STR         "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx"
#define MAC_STR_ARGS(m) m[0], m[1], m[2], m[3], m[4], m[5]

#define ETH_ALEN 6

int net_device_up(char *ifname);

int net_create_raw_socket(char *ifname, unsigned short proto, int non_blocking);

void net_print_packet(char *packet, unsigned size);

int net_get_iface_mac(char *ifname, char *mac_out);

int net_send(int socketfd, unsigned short eth_type, char *ifname, char *mac_addr, char *msg_buf, unsigned msg_size);
#endif
