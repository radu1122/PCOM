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
    string username;
    string password;
    int sockNr;
    bool isConnected;
};



// find in hashmap: auto topic = topics.find(data.topic);
        // if (subscriberData == subscribers.end()) {      
        //                 continue;
        //             }
// delete from hashmap: clientsConnected.erase(i);
// add elem hashmap: clients[x] = y;


// stream string: stringstream stream; // generare mesaj pentru client
                // stream << udpIp << ":" << server_addr.sin_port << " - ";
                // stream << data.topic << " - " << data.type << " - " << data.payload << "\n";

unordered_map<string, user> users;



int main(int argc, char *argv[]) {
    DIE(argc < 2, "Usage err");
	int PORT = atoi(argv[1]);
	DIE(PORT == 0, "port err");

    struct user user1;
    user1.username = "student";
    user1.password = "student";
    user1.sockNr = -1;
    user1.isConnected = false;

    struct user user2;
    user2.username = "test";
    user2.password = "test";
    user2.sockNr = -1;
    user2.isConnected = false;

    users[user2.username] = user2;

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    fd_set read_fds, tmp_fds;

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

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	FD_SET(0, &read_fds);
	FD_SET(sockTcp, &read_fds);

    int fd_max = sockTcp;

    while(1) {
        tmp_fds = read_fds;
        res = select(fd_max + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(res == -1, "fd select err");
	    char buf[MAX_LEN];

        if (FD_ISSET(STDIN, &tmp_fds)) { // verify stdin cmd
            memset(buf, 0, MAX_LEN);
            fgets(buf, MAX_LEN - 1, stdin);
            if (strcmp(buf, "exit\n") == 0) {
                unordered_map<string, user>::iterator it = users.begin();
                while(it != users.end()) { // inchide toate conexiunile si anunta clientii
                    memset(buf, 0, MAX_LEN);
                    strcpy(buf, ";EXIT;\n");
                    res = send(it->second.sockNr, buf, strlen(buf), 0);
                    DIE(res < 0, "UDP receive err");
                    close(it->second.sockNr);
                    it++;
                }
                break;
            }
        }

        for (int i = 1; i <= fd_max; i++) {
            if (i == sockTcp && FD_ISSET(i, &tmp_fds)) { // client nou
                int newsockFd = accept(sockTcp, (struct sockaddr *) &client_addr, (socklen_t *) &sockLen);
                DIE(newsockFd < 0, "TCP receive err");
                FD_SET(newsockFd, &read_fds);
                fd_max = max(fd_max, newsockFd);

                // memset(buf, 0, MAX_LEN); // only if needed
                // res = recv(newsockFd, buf, sizeof(buf), 0);

                
            } else if (FD_ISSET(i, &tmp_fds)) { // primire pe TCP de la clienti deja conectati
                memset(buf, 0, MAX_LEN);
                res = recv(i, buf, sizeof(buf), 0);
                DIE(res < 0, "tcp receive err");
                if (res == 0) { // beacon de disconnect
                    for (unordered_map<string,user>::iterator it=users.begin(); it!=users.end(); ++it) {
                        if (it->second.sockNr == i) {
                            struct user updatedUserClose;
                            updatedUserClose.username = it->second.username;
                            updatedUserClose.password = it->second.password;
                            updatedUserClose.sockNr = -1;
                            updatedUserClose.isConnected = false;

                            users[updatedUserClose.username] = updatedUserClose;

                            close(i);
                            FD_CLR(i, &read_fds);
                            break;
                        }

                    }
                }
                if (strncmp(buf, "login", 5) == 0) {
                    char username[50], password[50];
                    char command[16];

                    sscanf(buf,"%s %s %s", command, username, password);
                    auto searchUser = users.find(username);

                    if (searchUser == users.end()) {
                        memset(buf, 0, MAX_LEN);
                        strcpy(buf, ";NO_USERNAME;\n");
                        res = send(i, buf, strlen(buf), 0);
                        DIE(res < 0, "TCP send err");
                        continue;
                    }

                    if (searchUser->second.isConnected == true) {
                        memset(buf, 0, MAX_LEN);
                        strcpy(buf, ";USER_CONNECTED;\n");
                        res = send(i, buf, strlen(buf), 0);
                        DIE(res < 0, "TCP send err");
                        continue;
                    } else if (strcmp(password, searchUser->second.password.c_str()) != 0) {
                        memset(buf, 0, MAX_LEN);
                        strcpy(buf, ";USER_PASS;\n");
                        res = send(i, buf, strlen(buf), 0);
                        DIE(res < 0, "TCP send err");
                        continue;
                    }

                    // update user state to connected
                    struct user updatedUser;
                    updatedUser.username = searchUser->second.username;
                    updatedUser.password = searchUser->second.password;
                    updatedUser.sockNr = i;
                    updatedUser.isConnected = true;

                    users[updatedUser.username] = updatedUser;

                    printf("New client connected from %s:%d.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                }
                if (strncmp(buf, "unsubscribe", 11) == 0) { // pachet cu flag de unsubscribe
                    // TODO
                } 
            }
        }

    }

    close(sockTcp);

    return 0;
}
