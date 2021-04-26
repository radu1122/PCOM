#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "utils.h"
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


unordered_map<int, data> clientsConnected;
unordered_map<string, vector<subscriber>> topics;
unordered_map<string, subscriber> subscribers;

udp_packet processUdpPacket(string udpIp, int port, char *buf) {
    udp_packet pkt;
    uint16_t number;
    uint8_t power;
    stringstream payload;
    cout << buf << endl;
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

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    fd_set read_fds, tmp_fds;

    udp_packet inputPkt;

    struct sockaddr_in server_addr, client_addr;
	socklen_t sockLen = sizeof(struct sockaddr_in);

    memset((char *) &server_addr, 0, sizeof(server_addr));
	server_addr.sin_family      = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port        = htons((uint16_t)PORT);

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

        if (FD_ISSET(STDIN, &tmp_fds)) { // verify stdin cmd
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

        for (int i = 1; i <= fd_max; i++) {
            if (i == sockUDP && FD_ISSET(i, &tmp_fds)) {
                memset(buf, 0, MAX_LEN);
                socklen_t sockLen = sizeof(struct sockaddr_in);
                res = recvfrom(sockUDP, buf, MAX_LEN, 0, (struct sockaddr *) &server_addr, &sockLen);
                cout << buf << endl;
                DIE(res < 0, "UDP receive err");
	            char udpIp[16];
                inet_ntop(AF_INET, &(server_addr.sin_addr), udpIp, 16);

                struct udp_packet data;
                data = processUdpPacket(udpIp, server_addr.sin_port, buf);

                if (!data.valid) {
                    continue;
                }

                stringstream stream;
                stream << udpIp << ":" << server_addr.sin_port << " - ";
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
            } else if (i == sockTcp && FD_ISSET(i, &tmp_fds)) {
                int newsockFd = accept(sockTcp, (struct sockaddr *) &client_addr, (socklen_t *) &sockLen);
                DIE(newsockFd < 0, "TCP receive err");
                FD_SET(newsockFd, &read_fds);
                fd_max = max(fd_max, newsockFd);
                char tcpIp[16];
                inet_ntop(AF_INET, &(client_addr.sin_addr), tcpIp, 16);

                struct data clientCon;
                clientCon.ip_client_tcp   = tcpIp;
                clientCon.port_client_tcp = client_addr.sin_port;

                memset(buf, 0, MAX_LEN);
                res = recv(newsockFd, buf, sizeof(buf), 0);
                clientCon.client_ID = buf; // if err check this TODO

                auto subscriberCon = subscribers.find(string(buf));

                if (subscriberCon != subscribers.end() && subscriberCon->second.connected == true) { // exista deja client id in dataset
                    printf("Client %s already connected.\n", buf);
                    memset(buf, 0, MAX_LEN);
                    strcpy(buf, "ID_EXISTS");
                    res = send(newsockFd, buf, strlen(buf), 0);
                    DIE(res < 0, "TCP send err");
                    continue;
                }

                clientsConnected[newsockFd] = clientCon;

                printf("New client %s connected from %s:%d.\n", buf, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                auto subscriber = subscribers.find(buf);

                if (subscriber != subscribers.end()) {
                    subscriber->second.connected = true;
			    	subscriber->second.fd = newsockFd;
                } else {
                    struct subscriber subscriberNew;
                    subscriberNew.connected = true;
                    subscriberNew.fd = newsockFd;
                    subscriberNew.clientID = buf;
                    subscribers[buf] = subscriberNew;
                }

                clientCon = clientsConnected[newsockFd];
                string clientID = clientCon.client_ID;
                auto iterator = subscribers.find(clientID);
                if (iterator == subscribers.end()) {
                    continue;
                }
                
                for (int j = 0; j < iterator->second.packets.size(); j++) {
                    res = send(i, iterator->second.packets[j].c_str(), iterator->second.packets[j].size(), 0);
                }
                iterator->second.packets.clear();
            } else if (FD_ISSET(i, &tmp_fds)) {
                memset(buf, 0, MAX_LEN);
                res = recv(i, buf, sizeof(buf), 0);
                DIE(res < 0, "tcp receive err");
                if (res == 0) {
                    if (clientsConnected[i].client_ID.size() != 0) {
                        cout << "Client " << clientsConnected[i].client_ID << " disconnected.\n";

                        auto subscriber = subscribers.find(string(clientsConnected[i].client_ID));
                        if (subscriber != subscribers.end()) {
                            subscriber->second.connected = false;
                        }
                        clientsConnected.erase(i);

                        close(i);
                        FD_CLR(i, &read_fds);
                        continue;
                    }
                }
                if (strncmp(buf, "unsubscribe", 11) == 0) {
                    char topicStr[51];
                    char junk[16];

                    sscanf(buf,"%s %s", junk, topicStr);

                    auto topic = topics.find(topicStr);

                    if (topic == topics.end()) {
                        continue;
                    }

                    auto clientCon = clientsConnected.find(i);
                    if (clientCon == clientsConnected.end()) {
                        continue;
                    }

                    for (int j = 0; j < topic->second.size(); j++) {
                        if (clientCon->second.client_ID == topic->second[j].clientID) {
                            topic->second.erase(topic->second.begin() + i);
                            break;
                        }
                    }
                } else if (strncmp(buf, "subscribe", 9) == 0) {
                    char topicStr[51];
                    char junk[16];
                    int SFInt;
                    bool SF;

                    sscanf(buf,"%s %s %d", junk, topicStr, &SFInt);

                    if (SFInt == 0) {
                        SF = false;
                    } else {
                        SF = true;
                    }

                    auto clientCon = clientsConnected.find(i);
                    if (clientCon == clientsConnected.end()) {
                        continue;
                    }

                    auto topic = topics.find(topicStr);

                    bool found = false;

                    if (topic != topics.end()) {
                        for (subscriber subscriber : topic->second) {
                            if (subscriber.clientID == clientCon->second.client_ID) {
                                subscriber.SF = SF;
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            struct subscriber subscriberNew;
                            subscriberNew.clientID = clientCon->second.client_ID;
                            subscriberNew.SF = SF;
                            topic->second.push_back(subscriberNew);
                        }
                    } else {
                            vector<subscriber> subscribersNew;
                            struct subscriber subscriberNew;
                            subscriberNew.clientID = clientCon->second.client_ID;
                            subscriberNew.SF = SF;
                            subscribersNew.push_back(subscriberNew);
                            topics[topicStr] = subscribersNew;
                    }
                    
                }
                
            }
        }

    }

    close(sockTcp);
    close(sockUDP);

    return 0;
}
