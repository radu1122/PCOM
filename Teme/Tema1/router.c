#include <queue.h>
#include "skel.h"

int interfaces[ROUTER_NUM_INTERFACES];
struct route_table_entry *rtable;
int rtable_size;

struct arp_entry *arp_table;
int arp_table_len;

uint32_t ipToInt(char *ip) {

	unsigned int group3, group2, group1, group0;
	sscanf(ip, "%u.%u.%u.%u", &group3, &group2, &group1, &group0);
	return (group3 << 24) + (group2 << 16) + (group1 << 8) + group0;
}

int readRtable(struct route_table_entry *rtable, char fileName[100]){

	FILE *inputFile = fopen(fileName, "r");
	DIE(inputFile == NULL, "Failed to open rtable");

	char line[MAX_LEN];
	int i = 0;
	while(fgets(line, sizeof(line), inputFile)) {
		char group1[100], group2[100], group3[100], group4[100];

		sscanf(line, "%s %s %s %s", &group1, &group2, &group3, &group4);

		rtable[i].prefix = htonl(ipToInt(group1));
		rtable[i].next_hop = htonl(ipToInt(group2));
		rtable[i].mask = htonl(ipToInt(group3));
		rtable[i].interface = atoi(group4);
		
		i++;
	}

	fclose(inputFile);

	return i;
}

struct arp_entry *get_arp_entry(__u32 ip) {
    /* TODO 2: Implement */
    for (int i = 0; i < arp_table_len; i++) {
    	if(ip == arp_table[i].ip) {
    		return &arp_table[i];
		}
	}
	return NULL;
}

void parseArpTable() 
{
	FILE *f;
	fprintf(stderr, "Parsing ARP table\n");
	f = fopen("arp_table.txt", "r");
	DIE(f == NULL, "Failed to open arp_table.txt");
	char line[100];
	int i = 0;
	for(i = 0; fgets(line, sizeof(line), f); i++) {
		char ip_str[50], mac_str[50];
		sscanf(line, "%s %s", ip_str, mac_str);
		fprintf(stderr, "IP: %s MAC: %s\n", ip_str, mac_str);
		arp_table[i].ip = inet_addr(ip_str);
		int rc = hwaddr_aton(mac_str, arp_table[i].mac);
		DIE(rc < 0, "invalid MAC");
	}
	arp_table_len = i;
	fclose(f);
	fprintf(stderr, "Done parsing ARP table.\n");
}

int main(int argc, char *argv[])
{
	msg m;
	int rc;
	queue q = queue_create();
	init(argc - 2, argv + 2);

	rtable = malloc(sizeof(struct route_table_entry) * 66000);
	arp_table = malloc(sizeof(struct  arp_entry) * 100);
	DIE(rtable == NULL, "memory err");
	rtable_size = read_rtable(rtable);
	parse_arp_table();

	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		/* Students will write code here */
		struct ether_header *eth_hdr = (struct ether_header *) m.payload;
		struct iphdr *ip_hdr = (struct iphdr *) (m.payload + sizeof(struct ether_header));
		struct ether_arp *arp_hdr = (struct ether_arp * ) (m.payload + sizeof(struct ether_header));

		uint16_t arp = htons(0x806);
		uint16_t ipv4 = htons(0x800);

		if (eth_hdr->ether_type == arp) {

		}
	}
}
