#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <errno.h>

#include <netpacket/packet.h>
#include <arpa/inet.h>

#include <net/if.h>
#include <net/ethernet.h>

#include "myether.h"

int net_device_up(char *ifname)
{
	struct ifreq ifr;
	int ret, s;

	s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		return -1;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

	ret = ioctl(s, SIOCGIFFLAGS, &ifr);
	if (ret != 0) {
		perror("ioctl()");
		return -1;
	}

	close(s);
	return (ifr.ifr_flags & IFF_UP);
}

int net_get_iface_mac(char *ifname, char *mac_out)
{
	int s, ret;
	struct ifreq ifr;

	s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
	ret = ioctl(s, SIOCGIFHWADDR, &ifr);
	close(s);

	if (ret < 0) {
		return -1;
	}

	memcpy(mac_out, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
	return 0;
}

int net_create_raw_socket(char *ifname, unsigned short proto, int non_blocking)
{
	int sock_raw;
	struct ifreq ifr;
	struct sockaddr_ll sll;
	int fdflags;

	/* Open raw socket */
	sock_raw = socket(PF_PACKET, SOCK_RAW, htons(proto));
	if (sock_raw < 0) {
		perror("socket()");
		return -1;
	}

	if (!ifname) {
		return sock_raw;
	}

	/* Get interface index */
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
	if (ioctl(sock_raw, SIOCGIFINDEX, &ifr) < 0) {
		perror("ioctl()");
		goto bail;
	}

	memset(&sll, 0, sizeof(sll));
	sll.sll_family = AF_PACKET;
	sll.sll_ifindex = ifr.ifr_ifindex;
	sll.sll_protocol = htons(proto);

	/* Bind the raw socket to the interface specified */
	if (bind(sock_raw, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
		perror("bind()");
		goto bail;
	}

	if (non_blocking == 0) {
		return sock_raw;
	}

	/* Make socket non-blocking */
	fdflags = fcntl(sock_raw, F_GETFL);
	if (fdflags < 0) {
		perror("fcntl()");
		goto bail;
	}

	if (fcntl(sock_raw, F_SETFL, fdflags | O_NONBLOCK) < 0) {
		goto bail;
	}

	return sock_raw;

bail:
	close(sock_raw);
	return -1;
}

void net_print_packet(char *packet, unsigned size)
{
	unsigned it;
	struct ether_header *h = (struct ether_header *)packet;

	for (it = 0; it < size; it++) {
		unsigned char c = ((unsigned char *)packet)[it];
		fprintf(stdout, "%02X ", c);
		if (!((it + 1) % 4)) {
			fprintf(stdout, "   ");
		}
		if (!((it + 1) % 16)) {
			fprintf(stdout, "\n");
		}
	}
	fprintf(stdout, "\n");

	fprintf(stdout, "Source : ");
	fprintf(stdout, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx\n", h->ether_shost[0], h->ether_shost[1], h->ether_shost[2],
		h->ether_shost[3], h->ether_shost[4], h->ether_shost[5]);

	fprintf(stdout, "Dest   : ");
	fprintf(stdout, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx\n", h->ether_dhost[0], h->ether_dhost[1], h->ether_dhost[2],
		h->ether_dhost[3], h->ether_dhost[4], h->ether_dhost[5]);
}

int net_send(int socketfd, unsigned short eth_type, char *ifname, char *mac_addr, char *msg_buf, unsigned msg_size)
{
	int ret;

	/* Build packet */
	unsigned int packet_len = sizeof(struct ether_header) + msg_size;
	char raw_packet[packet_len];
	struct ether_header *eth_header = (struct ether_header *)raw_packet;
	net_get_iface_mac(ifname, (char *)eth_header->ether_shost);
	memset(eth_header->ether_dhost, 0, ETH_ALEN);
	eth_header->ether_type = htons(eth_type);

	/* Fill payload if present */
	if (msg_buf && (msg_size > 0)) {
		memcpy(raw_packet + sizeof(struct ether_header), msg_buf, msg_size);
	}

	/* Check against packet size TODO */

	memcpy(eth_header->ether_dhost, mac_addr, ETH_ALEN);

	ret = send(socketfd, (char*)raw_packet, packet_len, 0);
	return ret;
}
