#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <errno.h>
#include <libgen.h>
#include <time.h>
#include <netpacket/packet.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>

#include "myether.h"
/*****************************************************************************
 * C03:	  PRIVATE CONSTANT DEFINITIONS
 *****************************************************************************/
#define MAX_IFNAMSIZ 32
#define ETHER_TYPE   ((u_int16_t) 0x1234)
/*****************************************************************************
 * C04:   PRIVATE DATA TYPES
 *****************************************************************************/
struct tx_pack_hdr {
	struct ether_header eth;
	char data[0];
}__attribute__((packed));
/*****************************************************************************
 * C05:   PRIVATE DATA DECLARATIONS
 *****************************************************************************/

/*****************************************************************************
 * C06:   PRIVATE (LOCAL) FUNCTION PROTOTYPES
 *****************************************************************************/
static void usage();
/*****************************************************************************
 * C07:   GLOBAL DATA DECLARATIONS
 *****************************************************************************/

/*****************************************************************************
 * C08:   GLOBAL FUNCTIONS
 *****************************************************************************/

int main(int argc, char *argv[])
{
	int sfd;
	char ifname[MAX_IFNAMSIZ] = {0};
	int ret;
	int size;
	struct timespec sleep_time;
	struct tx_pack_hdr *hdr;
	char *buffer;
	char *arg_data;
	char *arg_ifname;

	if (argc != 3) {
		usage();
		goto bail;
	}

	arg_ifname = argv[1];
	arg_data = argv[2];

	snprintf(ifname, MAX_IFNAMSIZ, "%s", arg_ifname);

	if (!net_device_up(ifname)) {
		fprintf(stderr, "%s is not up\n", ifname);
		goto bail;
	}

	sfd = net_create_raw_socket(ifname, ETHER_TYPE, 0);
	if (sfd == -1) {
		fprintf(stderr, "failed to init socket\n");
		goto bail;
	}

	size = strlen(arg_data) + 1 + sizeof(*hdr);

	buffer = malloc(size);
	if (!buffer) {
		fprintf(stderr, "memory allocation error!\n");
		goto bail;
	}
	hdr = (struct tx_pack_hdr*)buffer;

	/*fill fix fields*/
	/* fill ethernet headers */
	net_get_iface_mac(ifname, (char *)hdr->eth.ether_shost);
	hdr->eth.ether_type = htons(ETHER_TYPE);
	memcpy(hdr->eth.ether_dhost, ETH_ADD_BCAST, ETH_ALEN);
	strcpy(hdr->data, arg_data);

	while (1) {
		ret = send(sfd, hdr, size, 0);
		if (ret != size) {
			fprintf(stderr, "ERROR: send failed ret: %d, errno: %d\n", ret, errno);
			goto bail;
		}
		fprintf(stderr, "%d bytes sent\n", ret);
		sleep_time.tv_sec = 1;
		sleep_time.tv_nsec = 0;
		nanosleep(&sleep_time, NULL);
	}

	return 0;

bail:
	return -1;
}
/*****************************************************************************
 * C09:   PRIVATE FUNCTIONS
 *****************************************************************************/
static void usage()
{
	fprintf(stderr, "\nUsage:\n./tx_raw <ifname> <string to send>\n");
	fprintf(stderr, "Example:\n./tx_raw eth0 hellooooo\n");
}
/*****************************************************************************
 * C10:   END OF CODE
 *****************************************************************************/
