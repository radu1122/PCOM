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

struct subscriber {
    string clientID;
	bool SF;
	bool connected;
	int fd;
	vector<string> packets;
};

struct user {
    int money;
    bool isConnected;
};

unordered_map<string, user> users;

int main(int argc, char *argv[]) {
    DIE(argc < 2, "Usage err");
	int PORT = atoi(argv[1]);
	DIE(PORT == 0, "port err");

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    fd_set read_fds, tmp_fds;

    struct sockaddr_in server_addr, client_addr;
    bzero(&server_addr, sizeof(server_addr));

    int sockUdp = socket(AF_INET, SOCK_DGRAM, 0);        
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    server_addr.sin_family = AF_INET; 
   
    bind(sockUdp, (struct sockaddr*)&server_addr, sizeof(server_addr));

    int res;
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	FD_SET(0, &read_fds);
	FD_SET(sockUdp, &read_fds);

    int fd_max = sockUdp;

    while(1) {
        tmp_fds = read_fds;
        res = select(fd_max + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(res == -1, "fd select err");
	    char buf[MAX_LEN];

        if (FD_ISSET(STDIN, &tmp_fds)) { // verify stdin cmd
            memset(buf, 0, MAX_LEN);
            fgets(buf, MAX_LEN - 1, stdin);
            if (strcmp(buf, "exit\n") == 0) {
                break;
            }
        }

        for (int i = 1; i <= fd_max; i++) {
            if (i == sockUdp && FD_ISSET(i, &tmp_fds)) { // primeste pe socketul de UDP
                memset(buf, 0, MAX_LEN);
                socklen_t sockLen = sizeof(struct sockaddr_in);
                res = recvfrom(sockUdp, buf, MAX_LEN, 0, (struct sockaddr *) &client_addr, &sockLen);
                DIE(res < 0, "UDP receive err");
                if (strncmp(buf, "CONNECT", 7) == 0) {
                    char id[50];
                    char junk[50];

                    sscanf(buf,"%s %s", junk, id);

                    auto searchUser = users.find(id);

                    if (searchUser == users.end()) {
                        struct user newUser;
                        newUser.isConnected = true;
                        newUser.money = 0;
                        users[id] = newUser;
                        continue;
                    }

                    if (searchUser->second.isConnected == true) {
                        memset(buf, 0, MAX_LEN);
                        strcpy(buf, ";USER_EXISTS;\n");
                        sendto(sockUdp, buf, strlen(buf), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
                        continue;
                    } else {
                        struct user newUser;
                        newUser.isConnected = true;
                        newUser.money = searchUser->second.money;
                        users[id] = newUser;
                        continue;
                    }
                } else if (strncmp(buf, "send_to", 7) == 0) {
                    char id[50], junk[50];
                    int sum = 0;
                    sscanf(buf,"%s %s %d", junk, id, &sum);

                    auto searchUser = users.find(id);

                    if (searchUser == users.end()) {
                        memset(buf, 0, MAX_LEN);
                        strcpy(buf, ";MONEY_NOSENT;\n");
                        sendto(i, buf, strlen(buf), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
                        continue;
                    }
                        struct user newUser;
                        newUser.isConnected = searchUser->second.isConnected;
                        newUser.money = searchUser->second.money + sum;
                        users[id] = newUser;

                        memset(buf, 0, MAX_LEN);
                        strcpy(buf, ";MONEY_SENT;\n");
                        sendto(i, buf, strlen(buf), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
                        continue;
                } else if (strncmp(buf, "show_sum", 8) == 0) {
                    char id[50], junk[50];
                    sscanf(buf,"%s %s", junk, id);

                    auto searchUser = users.find(id);

                    memset(buf, 0, MAX_LEN);

                    sprintf(buf, ";SHOW_SUM %d", searchUser->second.money);
                    sendto(sockUdp, buf, strlen(buf), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
                } else if (strncmp(buf, "get", 3) == 0) {
                    char id[50], junk[50];
                    int sum;
                    sscanf(buf,"%s %d %s", junk, &sum, id);

                    auto searchUser = users.find(id);

                    if (searchUser->second.money < sum) {
                        memset(buf, 0, MAX_LEN);
                        strcpy(buf, ";GET_ERROR;\n");
                        sendto(i, buf, strlen(buf), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
                        continue;
                    } else {
                        struct user newUser;
                        newUser.isConnected = searchUser->second.isConnected;
                        newUser.money = searchUser->second.money - sum;
                        users[id] = newUser;

                        memset(buf, 0, MAX_LEN);
                        sprintf(buf, ";GET_MONEY %d", sum);
                        sendto(i, buf, strlen(buf), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
                    }
                } else if (strncmp(buf, "exit", 4) == 0) {
                    char id[50], junk[50];
                    sscanf(buf,"%s %s", junk, id);

                    auto searchUser = users.find(id);

                    struct user newUser;
                    newUser.isConnected = false;
                    newUser.money = searchUser->second.money;
                    users[id] = newUser;
                }

            }
        }

    }

    close(sockUdp);

    return 0;
}
