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
#define RX_BUF_SIZE	1482
/*****************************************************************************
 * C04:   PRIVATE DATA TYPES
 *****************************************************************************/


struct rx_pack_hdr {
	struct ether_header eth;
	char name[10];
	char surname[10];
	char file_name[32];
	char file_size[4];
	uint16_t fragment_count;
	uint16_t fragment_index;
	uint32_t fragment_size;
	uint32_t crc;
	char data[0];
}__attribute__((packed));
/*****************************************************************************
 * C05:   PRIVATE DATA DECLARATIONS
 *****************************************************************************/

/*****************************************************************************
 * C06:   PRIVATE (LOCAL) FUNCTION PROTOTYPES
 *****************************************************************************/
static void usage();
uint32_t crc32_for_byte(uint32_t );
int control_flags(int*, uint16_t);
void crc32(const void*, size_t, uint32_t* );
void my_copy(char*,char*,int);
int write_buffer_to_file(char*, char**, uint16_t, uint32_t, uint32_t);
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
	//struct timespec sleep_time;
	struct rx_pack_hdr *hdr;
	char *buffer;
	char *arg_ifname;
    int isFirst = 1;
    int *flag;
    char **file;
    int general_counter = 0;
    uint32_t end_packet_size;
    uint32_t normal_packet_size;


    uint32_t* crc_ = (uint32_t*) calloc(1, sizeof(uint32_t));

	if (argc != 2) {
		usage();
		goto bail;
	}

	arg_ifname = argv[1];

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

	buffer = malloc(RX_BUF_SIZE);

	if (!buffer) {
		fprintf(stderr, "memory allocation error!\n");
		goto bail;
	}
	hdr = (struct rx_pack_hdr*)buffer;


    while (1) {
		ret = recv(sfd, hdr, RX_BUF_SIZE, 0);
        uint16_t frag_count = hdr->fragment_count;
        uint16_t frag_index = hdr->fragment_index;
        uint32_t frag_size = hdr->fragment_size;
        char* data;
        data = hdr->data;
        if (ret <= 0) {
			fprintf(stderr, "ERROR: recv failed ret: %d, errno: %d\n", ret, errno);
			goto bail;
		}
        if (ntohs(hdr->eth.ether_type) != ETHER_TYPE) {
            continue;
		}
        memset(crc_, 0 , sizeof(uint32_t));
        crc32(data, (size_t)(frag_size), crc_);
        if(*crc_ != hdr->crc){
            continue;
        }
        if(frag_size > RX_BUF_SIZE){
            continue;
        }
        if(isFirst){
            int i;

            flag = (int*)calloc(frag_count, sizeof(int));
            file  = (char**)calloc(frag_count, sizeof(char*));

            for(i = 0; i < frag_count; i++){
                file[i] = (char*) calloc(frag_size, sizeof(char));
            }
            memset(flag, 0, (frag_count) * sizeof(int));
            normal_packet_size = frag_size;
            isFirst = 0;
        }
        else {
            if (flag[frag_index-1] != 0) {
                continue;
            }
            flag[frag_index-1] = 1;

            general_counter++;


            my_copy(file[frag_index-1], data, frag_size);
            if(frag_index  == frag_count){
                end_packet_size = frag_size;
            }
            if(general_counter < frag_count){
                continue;
            }
            int control = control_flags(flag, frag_count);

            if(control){
                write_buffer_to_file("bira_gobegi.mp3", file, frag_count, normal_packet_size, end_packet_size);
                free(crc_);
                free(flag);
                int i;
                for(i = 0; i < frag_count; i++){
                    free(file[i]);
                }
                free(file);
                free(buffer);
                break;
            }
        }


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
	fprintf(stderr, "\nUsage:\n./rx_raw <ifname>\n");
	fprintf(stderr, "Example:\n./rx_raw eth0\n");
}


void my_copy(char* target,char* source, int size){
    int i;
    for(i = 0; i < size; i++){
        target[i] = source[i];
    }
}

int control_flags(int *ptr, uint16_t count){
    int i = 0;
    while(i <  count){
        if(!ptr[i]){
            return 0;
        }
        i++;
    }
    return 1;
}


int write_buffer_to_file(char *file_name, char **buffer, uint16_t count,uint32_t normal_packet_size, uint32_t end_packet_size)
{
	FILE *fp = NULL;
	int ret, i;

	fp = fopen((const char*)file_name, "w");

	if (!fp) {
		fprintf(stderr, "ERROR: cannot open file %s, errno: %d\n", file_name, errno);
		goto bail;
	}

    for(i = 0; i < count; i++){
        char* holder;
        holder = buffer[i];
        if(i != count - 1){
            ret = fwrite(holder, normal_packet_size, 1, fp);
        }
        else{
            ret = fwrite(holder, end_packet_size, 1, fp);
        }
    }

	if (ret < 1) {
		fprintf(stderr, "ERROR: fwrite failed ret: %d, errno: %d\n", ret, errno);
		goto bail;
	}

	return 0;
	bail:
	return -1;
}

uint32_t crc32_for_byte(uint32_t r)
{
	int j;
	for (j = 0; j < 8; ++j)
		r = (r & 1 ? 0 : (uint32_t)0xEDB88320L) ^ r >> 1;
	return r ^ (uint32_t)0xFF000000L;
}

void crc32(const void *data, size_t n_bytes, uint32_t* crc)
{
	size_t i;
	static uint32_t table[0x100];
	if (!*table)
		for (i = 0; i < 0x100; ++i)
			table[i] = crc32_for_byte(i);
	for (i = 0; i < n_bytes; ++i)
		*crc = table[(uint8_t)*crc ^ ((uint8_t*)data)[i]] ^ *crc >> 8;
}

/*****************************************************************************
 * C10:   END OF CODE
 *****************************************************************************/