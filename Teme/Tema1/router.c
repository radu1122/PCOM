#include <queue.h>
#include "skel.h"
#include <netinet/if_ether.h>

struct arp_entry {
	__u32 ip;
	uint8_t mac[6];
};

struct route_table_entry {
	uint32_t prefix;
	uint32_t next_hop;
	uint32_t mask;
	int interface;
} __attribute__((packed));

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
    int left = 0;
    int right = rtable_size - 1;
    int mid = 0;
	int finalId = -1;
    
    while( left <= right )
    {
        mid = (left + right) / 2;
        
		if ((dest_ip & rtable[mid].mask) == rtable[mid].prefix) {
			finalId = mid;
			break;
		} else if ((dest_ip & rtable[mid].mask) > rtable[mid].prefix) {
            right = mid - 1;
        } else if ((dest_ip & rtable[mid].mask) < rtable[mid].prefix) {
            left = mid + 1;
        }
    
    }
	if (finalId == -1) {
		return NULL;
	}
	return &rtable[finalId];
}

struct arp_entry *get_arp_entry(__u32 ip) {
    for (int i = 0; i < arp_table_len; i++) {
    	if(ip == arp_table[i].ip) {
    		return &arp_table[i];
		}
	}
	return NULL;
}

int comparator(const void *o1, const void *o2) {
    int o1Prefix = ((struct route_table_entry *)o1)->prefix;
    int o2Prefix = ((struct route_table_entry *)o2)->prefix; 

    return o2Prefix - o1Prefix; 
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
	qsort((void*)rtable, rtable_size, sizeof(struct route_table_entry), comparator);
    arp_table_len = 0;

	while (1) {

		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		struct ether_header *eth_hdr = (struct ether_header *) m.payload;
		struct iphdr *ip_hdr = (struct iphdr *) (m.payload + sizeof(struct ether_header));
		struct arp_header *arp_hdr = parse_arp(m.payload);

		uint16_t arp = htons(ETHERTYPE_ARP);
		uint16_t ipv4 = htons(0x800);

		if (eth_hdr->ether_type == arp) { // ARP
			if (arp_hdr->op == htons(ARPOP_REQUEST)) { // ARP request
				uint8_t *aux = malloc(6*sizeof(uint8_t));
				get_interface_mac(m.interface, aux);
				memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);
				memcpy(eth_hdr->ether_shost, aux, 6);
				eth_hdr->ether_type = htons(ETHERTYPE_ARP);

				send_arp(arp_hdr->spa, htonl(ipToInt(get_interface_ip(m.interface))), eth_hdr, m.interface, htons(ARPOP_REPLY));
				
			} else { // ARP reply

				struct arp_entry newArp;
				newArp.ip = arp_hdr->spa;
				memcpy(newArp.mac, arp_hdr->sha, 6);

				arp_table_len++;	
				arp_table[arp_table_len - 1] = newArp;
					
				queue q2 = queue_create();
				while(!queue_empty(q)) { // verifica toata pachetele din coada
					packet * firstPacket = (packet *) queue_deq(q);
					struct iphdr *ip_hdr1 = (struct iphdr *) (firstPacket->payload + sizeof(struct ether_header)); 	

					struct route_table_entry *bestRoute = get_best_route(ip_hdr1->daddr);

					if (bestRoute->next_hop != arp_hdr->spa) {
						queue_enq(q2, firstPacket);
					} else {
						ip_hdr1->ttl = ip_hdr1->ttl - 1;
						ip_hdr1->check = 0;
						ip_hdr1->check = ip_checksum(ip_hdr1, sizeof(struct iphdr));
						
						struct ether_header *eth_hdr1 = (struct ether_header *) firstPacket->payload;

						memcpy(eth_hdr1->ether_dhost, arp_hdr->sha, 6);


						get_interface_mac(bestRoute->interface, eth_hdr1->ether_shost);
						send_packet(bestRoute->interface, firstPacket);
					}

				}

				// q = q2;
				while (queue_empty(q2) != 1) {
					packet firstPacket = * (packet *) queue_deq(q2);
					queue_enq(q, &firstPacket);
				}
			}
			
		} else
		 if (eth_hdr->ether_type == ipv4) {
			struct route_table_entry *route = get_best_route(ip_hdr->daddr);

      		uint16_t currSum = ip_hdr->check;
			ip_hdr->check = 0;
			uint16_t newSum = ip_checksum(ip_hdr, sizeof(struct iphdr));

			if (newSum != currSum) { // checksum err, drop packet
				continue;
			};

			if (ip_hdr->ttl <= 1) { // drop packet tle
				send_icmp_error(ip_hdr->saddr, ip_hdr->daddr, eth_hdr->ether_dhost, eth_hdr->ether_shost, ICMP_TIME_EXCEEDED, ICMP_NET_UNREACH, m.interface);
				continue;
			}

			if (route == NULL) { // route unknown
				send_icmp_error(ip_hdr->saddr, ip_hdr->daddr, eth_hdr->ether_dhost, eth_hdr->ether_shost, ICMP_DEST_UNREACH, ICMP_NET_UNREACH, m.interface);

				continue;
			}

			if (ip_hdr->protocol == 1) { // echo packet
				struct icmphdr *icmp_hdr = parse_icmp(m.payload);
				struct in_addr routerIp;

				char *routerIpString = get_interface_ip(m.interface);
				inet_aton(routerIpString, &routerIp);
				if (icmp_hdr->type == ICMP_ECHO && routerIp.s_addr == ip_hdr->daddr) {
					send_icmp(ip_hdr->saddr, ip_hdr->daddr, eth_hdr->ether_dhost, eth_hdr->ether_shost, 0, 0, m.interface, htons(getpid()), 0);
					continue;
				}
			}

			struct arp_entry *arpX = get_arp_entry(ip_hdr->daddr);

			if (arpX == NULL) { // destination mac unknown
				packet * buff = malloc(sizeof(m));
				memcpy(buff, &m, sizeof(m));
				queue_enq(q, buff);

				uint8_t *aux = malloc(6*sizeof(uint8_t));
				get_interface_mac(route->interface, aux);
				memcpy(eth_hdr->ether_shost, aux, 6);
				memset(eth_hdr->ether_dhost, 0xff, 6);
				eth_hdr->ether_type = 1544;

				struct in_addr x;

				inet_aton(get_interface_ip(route->interface), &x);

				send_arp(route->next_hop, x.s_addr, eth_hdr, route->interface, htons(ARPOP_REQUEST));

			} else { // forward packet
				ip_hdr->ttl = ip_hdr->ttl - 1;
				ip_hdr->check = 0;
				ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));

				memcpy(eth_hdr->ether_dhost, &arpX->mac, 6);

				send_packet(route->interface, &m);
				continue;
			}
		}
	}
}
