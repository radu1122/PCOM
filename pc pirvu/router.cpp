#include "include/rtable.h"
#include "include/skel.h"
#include <netinet/if_ether.h>
#include <queue>
#include <unordered_map>

#define ARP_REQUEST_CODE 1
#define ARP_REPLY_CODE 2
#define ARP_P_IP 0x0800
#define ARP_HW_ETHER 0x0001

struct ARPEntry {
  uint32_t ip;
  uint8_t mac[ETH_ALEN];

  ARPEntry() {
    ip = 0;
    memset(&mac, 0, sizeof(uint8_t) * ETH_ALEN);
  }

  bool operator<(const ARPEntry &other) const { return ip < other.ip; }
};

/**
 * Sends ARP Reply to a received ARP Request.
 *
 * @param arpRequest Incoming request
 * @param newMac Mac address to send back
 */
void sendARPReply(packet *arpRequest, uint8_t *newMac) {
  packet *sent = new packet;

  // get headers of request
  struct ether_header *eth_hdr = (struct ether_header *)arpRequest->payload;
  struct ether_arp *arp_hdr =
      (struct ether_arp *)(arpRequest->payload + sizeof(struct ether_header));

  // populate ether header of reply
  struct ether_header *sent_eth_hdr = (struct ether_header *)sent->payload;
  memcpy(sent_eth_hdr->ether_dhost, eth_hdr->ether_shost,
         sizeof(uint8_t) * ETH_ALEN);
  memcpy(sent_eth_hdr->ether_shost, newMac, sizeof(uint8_t) * ETH_ALEN);
  sent_eth_hdr->ether_type = eth_hdr->ether_type;
  
  // populate arp header of reply
  struct ether_arp *sent_arp_hdr =
      (struct ether_arp *)(sent->payload + sizeof(struct ether_header));
  memcpy(sent_arp_hdr, arp_hdr, sizeof(struct ether_arp));
  sent_arp_hdr->ea_hdr.ar_op = htons(ARP_REPLY_CODE);
  memcpy(sent_arp_hdr->arp_tpa, arp_hdr->arp_spa, sizeof(uint8_t) * 4);
  memcpy(sent_arp_hdr->arp_tha, arp_hdr->arp_sha, ETH_ALEN * sizeof(uint8_t));
  memcpy(sent_arp_hdr->arp_spa, arp_hdr->arp_tpa, sizeof(uint8_t) * 4);
  memcpy(sent_arp_hdr->arp_sha, newMac, sizeof(uint8_t) * ETH_ALEN);
  
  // prepare packet and send
  sent->interface = arpRequest->interface;
  sent->len = arpRequest->len;
  send_packet(sent->interface, sent);
}

/**
 * Sends ARP Request from router to host.
 *
 * @param entry Longest prefix match entry from routing table
 * @param daddr Destination address where request will be sent
 */
void sendARPRequest(rTableEntry entry, uint32_t *daddr) {
  packet *sent = new packet;
  
  // populate ether header of request
  struct ether_header *sent_eth_hdr = (struct ether_header *)sent->payload;
  sent_eth_hdr->ether_type = htons(ETHERTYPE_ARP);

  // send on broadcast
  hwaddr_aton("ff:ff:ff:ff:ff:ff", sent_eth_hdr->ether_dhost);
  get_interface_mac(entry.interface, sent_eth_hdr->ether_shost);

  // populate arp header of request
  struct ether_arp *sent_arp_hdr =
      (struct ether_arp *)(sent->payload + sizeof(struct ether_header));
  sent_arp_hdr->ea_hdr.ar_hrd = htons(ARP_HW_ETHER);
  sent_arp_hdr->ea_hdr.ar_pro = htons(ARP_P_IP);
  sent_arp_hdr->ea_hdr.ar_pln = 4;
  sent_arp_hdr->ea_hdr.ar_hln = 6;
  sent_arp_hdr->ea_hdr.ar_op = htons(ARP_REQUEST_CODE);
  

  // populate source with router info
  in_addr sourceIp;
  char *routerIpString = get_interface_ip(entry.interface);
  inet_aton(routerIpString, &sourceIp);

  memcpy(&sent_arp_hdr->arp_spa, &sourceIp, 4 * sizeof(uint8_t));
  get_interface_mac(entry.interface, sent_arp_hdr->arp_sha);
  
  // target mac will be populated by host on reply
  memcpy(&sent_arp_hdr->arp_tpa, daddr, 4 * sizeof(uint8_t));
  hwaddr_aton("00:00:00:00:00:00", sent_arp_hdr->arp_tha);
  
  // prepare packet and send
  sent->interface = entry.interface;
  sent->len = sizeof(ether_header) + sizeof(ether_arp);

  send_packet(entry.interface, sent);
}

/**
 * Sends ICMP echo reply from router.
 *
 * @param packet Packet containing ICMP echo request.
 */
void sendICMPReply(packet &m) {
  // get ICMP echo request headers
  ether_header *eth_hdr = (struct ether_header *)m.payload;
  icmphdr *icmp_hdr =
      (icmphdr *)(m.payload + sizeof(ether_header) + sizeof(iphdr));
  iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
  
  // set correct reply packet info
  icmp_hdr->type = 0;
  icmp_hdr->code = 0;

  swap(ip_hdr->daddr, ip_hdr->saddr);
  ip_hdr->ttl = 64;
  
  // update checksums
  ip_hdr->check = 0;
  ip_hdr->check = ip_checksum(ip_hdr, sizeof(iphdr));
  icmp_hdr->checksum = 0;
  icmp_hdr->checksum = ip_checksum(icmp_hdr, sizeof(icmphdr));

  send_packet(m.interface, &m);
}

/**
 * Sends ICMP packet built from scractch, using information from a received
 * packet.
 *
 * @param m Last received packet that triggered this call. (not necessarily ICMP)
 * @param type Type field of ICMP header
 * @param code Code field of ICMP header
 */
void sendICMPscratch(packet &m, int type, int code) {
  packet sent;
  
  // get headers of packet we want to send
  ether_header *eth_hdr = (struct ether_header *)sent.payload;
  icmphdr *icmp_hdr =
      (icmphdr *)(sent.payload + sizeof(ether_header) + sizeof(iphdr));
  iphdr *ip_hdr = (struct iphdr *)(sent.payload + sizeof(struct ether_header));

  // get headers of last received packet
  ether_header *m_eth_hdr = (struct ether_header *)m.payload;
  iphdr *m_ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
  
  // set correct length
  sent.len = sizeof(struct ether_header) + sizeof(struct iphdr) +
             sizeof(struct icmphdr);
  
  // populate ether header reversing source and target
  eth_hdr->ether_type = htons(ETHERTYPE_IP);
  memcpy(eth_hdr->ether_dhost, m_eth_hdr->ether_shost,
         sizeof(uint8_t) * ETH_ALEN);
  memcpy(eth_hdr->ether_shost, m_eth_hdr->ether_dhost,
         sizeof(uint8_t) * ETH_ALEN);
  
  // populate IP with correct fields & calculate checksum
  ip_hdr->version = 4;
  ip_hdr->ihl = 5;
  ip_hdr->tos = 0;
  ip_hdr->tot_len = htons(sent.len - sizeof(struct ether_header));
  ip_hdr->id = htons(getpid());
  ip_hdr->frag_off = 0;
  ip_hdr->ttl = 64;
  ip_hdr->protocol = 1;
  ip_hdr->saddr = m_ip_hdr->daddr;
  ip_hdr->daddr = m_ip_hdr->saddr;
  ip_hdr->check = 0;
  ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));
  
  // populate ICMP with correct fields & calculate checksum
  icmp_hdr->code = code;
  icmp_hdr->type = type;
  icmp_hdr->un.echo.id = htons(getpid());
  icmp_hdr->un.echo.sequence = 0;
  icmp_hdr->checksum = 0;
  icmp_hdr->checksum = ip_checksum(icmp_hdr, sizeof(struct icmphdr));

  send_packet(m.interface, &sent);
}

int main(int argc, char *argv[]) {
  packet m;
  int rc;
  unordered_map<uint32_t, ARPEntry> arpTable;
  queue<packet> q;

  rTable rt("rtable.txt");
  rt.parseRTable();

  init();

  while (1) {
    rc = get_packet(&m);
    DIE(rc < 0, "get_message");

    // get router ip for destination checks
    in_addr routerIp;

    char *routerIpString = get_interface_ip(m.interface);
    inet_aton(routerIpString, &routerIp);

    // get ether header and check ether type
    struct ether_header *eth_hdr = (struct ether_header *)m.payload;

    if (ntohs(eth_hdr->ether_type) == ETHERTYPE_ARP) {
      // received ARP packet
      struct ether_arp *arp_hdr =
          (struct ether_arp *)(m.payload + sizeof(struct ether_header));

      if (ntohs(arp_hdr->ea_hdr.ar_op) == ARP_REQUEST_CODE) {
        // received ARP request
        uint8_t newMac[ETH_ALEN];
        unsigned int incomingIp;

        memcpy(&incomingIp, arp_hdr->arp_tpa, 4 * sizeof(uint8_t));

        if (routerIp.s_addr == incomingIp) {
          // ARP request for router
          get_interface_mac(m.interface, newMac);
          sendARPReply(&m, newMac);
        } else {
          // host asked for another host's mac
          unsigned int destIp;
          memcpy(&destIp, arp_hdr->arp_tpa, 4 * sizeof(uint8_t));

          // lookup routing table and send if found
          if (arpTable.find(destIp) != arpTable.end()) {
            sendARPReply(&m, arpTable[destIp].mac);
          }
        }
      } else if (ntohs(arp_hdr->ea_hdr.ar_op) == ARP_REPLY_CODE) {
        // received reply, update ARP table
        ARPEntry newArpEntry;
        memcpy(&newArpEntry.ip, arp_hdr->arp_spa, 4 * sizeof(uint8_t));
        memcpy(&newArpEntry.mac, arp_hdr->arp_sha, ETH_ALEN * sizeof(uint8_t));
        arpTable[newArpEntry.ip] = newArpEntry;

        // empty queue and send packets that were waiting for this reply
        while (!q.empty()) {
          packet currentPacket = q.front();
          q.pop();
          struct ether_header *q_eth_hdr =
              (struct ether_header *)currentPacket.payload;
          struct iphdr *q_ip_hdr =
              (struct iphdr *)(currentPacket.payload +
                               sizeof(struct ether_header));

          // update ttl and checksum
          q_ip_hdr->ttl = q_ip_hdr->ttl - 1;
          q_ip_hdr->check = 0;
          q_ip_hdr->check = ip_checksum(q_ip_hdr, sizeof(struct iphdr));
          
          // copy new mac
          for (int i = 0; i < 6; ++i) {
            q_eth_hdr->ether_dhost[i] = arpTable[q_ip_hdr->daddr].mac[i];
          }
          
          // find longest prefix match and forward
          rTableEntry bestMatch = rt.findBestEntry(q_ip_hdr->daddr);
          send_packet(bestMatch.interface, &currentPacket);
        }
      }
    } else if (ntohs(eth_hdr->ether_type) == ETHERTYPE_IP) {
      // received IP packet, get header and find longest prefix match
      struct iphdr *ip_hdr =
          (struct iphdr *)(m.payload + sizeof(struct ether_header));
      rTableEntry bestMatch = rt.findBestEntry(ip_hdr->daddr);
    
      // get old checksum to avoid overwrite
      uint16_t oldChecksum = ip_hdr->check;
      ip_hdr->check = 0;

      if (ip_checksum(ip_hdr, sizeof(struct iphdr)) != oldChecksum) {
        printf("Dropping packet. Reason: Invalid checksum\n");
        continue;
      };

      if (ip_hdr->ttl < 1) {
        printf("Dropping packet. Reason : Invalid TTL\n");
        continue;
      } else if (ip_hdr->ttl == 1) {
        // send timeout (ttl would be 0 after forward)
        sendICMPscratch(m, 11, 0);
        continue;
      }

      if (bestMatch.prefix == 0) {
        // no entry found in routing table
        sendICMPscratch(m, 3, 0);
        continue;
      }

      if (ip_hdr->protocol == 1) {
        // ip contains ICMP header
        icmphdr *icmp_hdr =
            (icmphdr *)(m.payload + sizeof(ether_header) + sizeof(iphdr));

        if (icmp_hdr->type == 8 && routerIp.s_addr == ip_hdr->daddr) {
          // icmp request for router
          sendICMPReply(m);
          continue;
        }
      }
      
      // check if we have the mac address of destination
      if (arpTable.find(ip_hdr->daddr) == arpTable.end()) {
        // ARP entry not found, make request and push packet to queue
        arpTable[ip_hdr->daddr] = ARPEntry();
        sendARPRequest(bestMatch, &ip_hdr->daddr);
        q.push(m);
      } else {
        // ARP entry found, update header and forward
        ip_hdr->ttl = ip_hdr->ttl - 1;
        ip_hdr->check = 0;
        ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));

        for (int i = 0; i < 6; ++i) {
          eth_hdr->ether_dhost[i] = arpTable[ip_hdr->daddr].mac[i];
        }

        send_packet(bestMatch.interface, &m);
        continue;
      }
    }
  }
}
