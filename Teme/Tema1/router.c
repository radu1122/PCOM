#include <queue.h>
#include "skel.h"
#include <netinet/if_ether.h>

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

		sscanf(line, "%s %s %s %s", group1, group2, group3, group4);

		rtable[i].prefix = htonl(ipToInt(group1));
		rtable[i].next_hop = htonl(ipToInt(group2));
		rtable[i].mask = htonl(ipToInt(group3));
		rtable[i].interface = atoi(group4);
		
		i++;
	}

	fclose(inputFile);

	return i;
}

struct route_table_entry *get_best_route(__u32 dest_ip) {
	/* TODO 1: Implement the function */
	int id = -1;
	int maskMax = 0;
	for (int i = 0; i < rtable_size; i++) {
		if ((dest_ip & rtable[i].mask) == rtable[i].prefix && rtable[i].mask > maskMax) {
			id = i;
			maskMax = rtable[i].mask;
		}
	}
	if (id == -1) {
		return NULL;
	}
	return &rtable[id];
}


struct arp_entry *get_arp_entry(__u32 ip) {
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
	packet m;
	int rc;
	queue q = queue_create();
	init(argc - 2, argv + 2);

	rtable = malloc(sizeof(struct route_table_entry) * 66000);
	arp_table = malloc(sizeof(struct  arp_entry) * 66000);
	DIE(rtable == NULL, "memory err");
	rtable_size = readRtable(rtable, argv[1]);
	// parseArpTable();
    arp_table_len = 0;
	while (1) {


		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		/* Students will write code here */
		struct ether_header *eth_hdr = (struct ether_header *) m.payload;
		struct iphdr *ip_hdr = (struct iphdr *) (m.payload + sizeof(struct ether_header));
		struct arp_header *arp_hdr = parse_arp(m.payload);

		uint16_t arp = htons(ETHERTYPE_ARP);
		uint16_t ipv4 = htons(0x800);

		if (eth_hdr->ether_type == arp) { // ARP
			
			if (arp_hdr->op == htons(1)) { // ARP request

				uint8_t *aux = malloc(6*sizeof(uint8_t));
				get_interface_mac(m.interface, aux);
				memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);
				memcpy(eth_hdr->ether_shost, aux, 6);
				eth_hdr->ether_type = htons(ETHERTYPE_ARP);

				send_arp(arp_hdr->spa, htonl(ipToInt(get_interface_ip(m.interface))), eth_hdr, m.interface, htons(2));
				
			} else { // ARP reply
				struct arp_entry newArp;
				memcpy(&newArp.ip, &(arp_hdr->spa), 4);
				memcpy(newArp.mac, arp_hdr->sha, 6);

				arp_table_len++;	
				arp_table[arp_table_len - 1] = newArp;
					
				queue q2 = queue_create();

				while(queue_empty(q) != 1) { // verifica toata pachetele din coada
					packet firstPacket = * (packet *) queue_deq(q);
					struct iphdr *ip_hdr1 = (struct iphdr *) (firstPacket.payload + sizeof(struct ether_header)); 	
					struct arp_entry *newMac = get_arp_entry(ip_hdr1->daddr);

					if (newMac == NULL) {
						queue_enq(q2, &firstPacket);
					} else {
						struct ether_header *eth_hdr1 = (struct ether_header *) firstPacket.payload;
						memcpy(eth_hdr1->ether_dhost, newMac, 6);
						send_packet(m.interface, &firstPacket);
					}

				}

				q = q2;
			}
			
		}

		
	}
}
