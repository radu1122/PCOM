#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <utils.h>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <math.h>

using namespace std;

struct udp_packet {
	int port_client_udp;
	string ip_client_udp;
	string type;
	string payload;
	string topic;
	bool valid;
};


struct subscriber {
    string clientID;
	bool SF;
	bool connected;
	int fd;
	vector<string> packets;
};

struct data {
	string ip_client_tcp;
	int port_client_tcp;
	string client_ID;
};


unordered_map<int, data> client_connected;
unordered_map<string, vector<subscriber>> topics;
unordered_map<string, subscriber> subscribers;

udp_packet processUdpPacket(string udpIp, int port, char *buf) {
    udp_packet pkt;
    uint16_t number;
    uint8_t power;
    stringstream payload;

	pkt.valid = true;

    switch(buf[TOPIC_SIZE]) {
    case 0:
		memcpy(&number, buf + TOPIC_SIZE + 2, sizeof(uint32_t));
        		number = ntohl(number);
		pkt.payload = to_string(number);

        if (buf[TOPIC_SIZE + 1] == 1) {
			pkt.payload.insert(0, "-");
		}
        
		pkt.type = "INT";

        break;
    case 1:
		memcpy(&number, buf + TOPIC_SIZE + 1, sizeof(uint16_t));
		number = ntohs(number);

		payload << fixed << setprecision(2) << 1.0 * number / 100;

		pkt.payload = payload.str();
        pkt.type = "SHORT REAL";
        break;
    case 2:
        memcpy(&number, buf + TOPIC_SIZE + 2, sizeof(uint32_t));
		number = ntohl(number);
        memcpy(&power, buf + TOPIC_SIZE + 2 + sizeof(uint32_t), sizeof(uint8_t));

        if (buf[TOPIC_SIZE + 1] == 1) {
			payload << "-";
		}
		payload << fixed << setprecision((int) power) << 1.0 * number / pow(10, (int) power);

		pkt.payload = payload.str();
        pkt.type = "FLOAT";
        break;
    case 3:
		pkt.payload = buf + TOPIC_SIZE + 1;
		pkt.type = "STRING";
        break;
    default:
        pkt.valid = false;
		return pkt;
    }

    buf[50] = '\0';

	char *token;
	token = strtok(buf, "\0");
	string topic = token;
	pkt.topic = topic;

    return pkt;
}


int main(int argc, char *argv[]) {
    DIE(argc < 2, "Usage err");
	int PORT = atoi(argv[1]);
	DIE(PORT == 0, "port err");

    fd_set read_fds, tmp_fds;

    udp_packet inputPkt;

    struct sockaddr_in server_addr, client_addr;
	socklen_t len = sizeof(struct sockaddr_in);

    memset((char *) &server_addr, 0, sizeof(server_addr));
	server_addr.sin_family      = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port        = htons(PORT);

    int sockTcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockTcp < 0, "tcp sock open err");

    // disable neagle alg
    int neagle = 1;
    int res = setsockopt(sockTcp, IPPROTO_TCP, TCP_NODELAY, &neagle, sizeof(neagle));
    DIE(res == -1, "disable neagle err");

    res = bind(sockTcp, (struct sockaddr *) &server_addr, sizeof(struct sockaddr));
    DIE(res == -1, "sock tcp bind err");

    res = listen(sockTcp, MAX_CLIENTS);
    DIE(res == -1, "tcp listen start err");

    int sockUDP = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(sockUDP < 0, "udp sock open err");

    res = bind(sockUDP, (struct sockaddr *) &server_addr, sizeof(server_addr));
    DIE(res == -1, "sock udp bind err");

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	FD_SET(0, &read_fds);
	FD_SET(sockTcp, &read_fds);
	FD_SET(sockUDP, &read_fds);

    int fd_max = max(sockTcp, sockUDP);

    while(1) {
        tmp_fds = read_fds;
        res = select(fd_max + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(res == -1, "fd select err");
	    char buf[MAX_LEN];

        if (FD_ISSET(0, &tmp_fds)) { // verify stdin cmd
            memset(buf, 0, MAX_LEN);
            fgets(buf, MAX_LEN - 1, stdin);
            if (strcmp(buf, "exit\n") == 0) {
                for (int k = 0; k <= fd_max; k++) {
                    if (FD_ISSET(k, &tmp_fds)) {
                        close(k);
                    }
                }
                break;
            }
        }

        for (int i = 0; i <= fd_max; i++) {
            if (i == sockUDP) {
                memset(buf, 0, MAX_LEN);
                socklen_t len = sizeof(struct sockaddr_in);
                res = recvfrom(sockUDP, buf, MAX_LEN, 0, (struct sockaddr *) &server_addr, &len);
                DIE(res < 0, "UDP receive err");
	            char udpIp[16];
                inet_ntop(AF_INET, &(server_addr.sin_addr), udpIp, 16);

                struct udp_packet data;
                data = processUdpPacket(udpIp, server_addr.sin_port, buf);

                if (!data.valid) {
                    continue;
                }

                stringstream stream;
                stream << data.ip_client_udp << ":" << data.port_client_udp << " - ";
                stream << data.topic << " - " << data.type << " - " << data.payload << "\n";

                auto topic = topics.find(data.topic);
                if (topic == topics.end()) {
                    continue;
                }

                for (subscriber subscriber : topic->second) {
                    auto subscriberData = subscribers.find(subscriber.clientID);
                    if (subscriberData == subscribers.end()) {
                        continue;
                    }

                    if (!subscriberData->second.connected && subscriber.SF) {
                        subscriberData->second.packets.push_back(stream.str());
                    }

                    if (subscriberData->second.connected) {
                        res = send(subscriberData->second.fd, stream.str().c_str(), stream.str().size(), 0);
                        DIE(res < 0, "tcp send err");
                    }
                }
            }
        }

    }

    close(sockTcp);
    close(sockUDP);

    return 0;
}
