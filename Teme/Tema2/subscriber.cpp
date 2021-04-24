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


using namespace std;


int main(int argc, char *argv[]) {

    DIE(argc < 4, "incorrect usage of the subscriber");

    char * ID = strndup(argv[1], ID_LEN);
	int PORT = atoi(argv[3]);

    DIE(PORT == 0, "Incorrect port");

    int sockTCP, fdMax, ret;

    sockTCP = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockTCP < 0, "socket open err");

    struct sockaddr_in servAddr;
    char buf[MAX_LEN];

    fd_set clientFd, tmpFd;

    FD_ZERO(&clientFd);
    FD_ZERO(&tmpFd);

    int neagle = 1;
    setsockopt(sockTCP, IPPROTO_TCP, TCP_NODELAY, &neagle, sizeof(int));

    servAddr.sin_family = AF_INET;
	servAddr.sin_port   = htons(PORT);
    ret = inet_aton(argv[2], &servAddr.sin_addr);

    ret = connect(sockTCP, (struct sockaddr *)&servAddr, sizeof(servAddr));
    DIE(ret < 0, "socket connect err");

    memset(buf, 0, MAX_LEN);
    strcpy(buf, argv[1]);

    ret = send(sockTCP, buf, strlen(buf), 0);
    DIE(ret < 0, "send id failed");

    while (true) {
        tmpFd = clientFd;

        ret = select(sockTCP + 1, &tmpFd, NULL, NULL, NULL);
        DIE(ret < 0, "sock select err");
    
    
        if (FD_ISSET(STDIN, &tmpFd)) {
            memset(buf, 0, MAX_LEN);
            fgets(buf, MAX_LEN - 1, stdin);
            
            if (strncmp(buf, "subscribe", 9) == 0) {
                ret = send(sockTCP, buf, strlen(buf), 0);
                DIE(ret < 0, "send cmd failed");
                printf("Subscribed to topic.\n");
            }

            if (strncmp(buf, "unsubscribe", 11) == 0) {
                ret = send(sockTCP, buf, strlen(buf), 0);
                DIE(ret < 0, "send cmd failed");
                printf("Unsubscribed from topic.\n");
            }

            if (strncmp(buf, "exit", 4) == 0) {
                ret = send(sockTCP, buf, strlen(buf), 0);
                DIE(ret < 0, "send exit failed");
                break;
            }
        }

        if (FD_ISSET(sockTCP, &tmpFd)) {
            memset(buf, 0, MAX_LEN);
            ret = recv(sockTCP, buf, MAX_LEN, 0);
            DIE(ret < 0, "sock read failed");

            if (ret == 0) {
                break;
            }

            if (strcmp(buf, "ID_EXISTS") == 0) {
                fprintf(stderr, "ID already exists\n");
                break;
            } else {
                printf("%s\n", buf);
            }
        }
    }

    close(sockTCP);
    return 0;
}