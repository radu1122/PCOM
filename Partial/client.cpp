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
#include <iostream>


using namespace std;


int main(int argc, char *argv[]) {

    DIE(argc < 2, "incorrect usage of the subscriber");

	int PORT = atoi(argv[2]);

    DIE(PORT == 0, "Incorrect port");

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int sockUdp, ret;

    sockUdp = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(sockUdp < 0, "socket open err");

    struct sockaddr_in servAddr, cliAddr;
    char buf[MAX_LEN];

    fd_set clientFd, tmpFd;

    memset(&servAddr, 0, sizeof(servAddr));
    memset(&cliAddr, 0, sizeof(cliAddr));
    FD_ZERO(&clientFd);
    FD_ZERO(&tmpFd);

    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAddr.sin_port = htons(PORT);
    servAddr.sin_family = AF_INET;

    ret = connect(sockUdp, (struct sockaddr *)&servAddr, sizeof(servAddr));
    DIE(ret < 0, "socket connect err");

    FD_SET(sockUdp, &clientFd);
    FD_SET(0, &clientFd);

    sprintf(buf, "CONNECT %s", argv[1]);

    sendto(sockUdp, (const char *)buf, strlen(buf), 0, (struct sockaddr*)NULL, sizeof(servAddr));

    while (1) {
        tmpFd = clientFd;

        ret = select(sockUdp + 1, &tmpFd, NULL, NULL, NULL);
        DIE(ret < 0, "sock select err");
    

        if (FD_ISSET(sockUdp, &tmpFd)) { // receive pe UDP
            memset(buf, 0, MAX_LEN);
            recvfrom(sockUdp, buf, sizeof(buf), 0, (struct sockaddr*)NULL, NULL);
            if (strncmp(buf, ";USER_EXISTS;", 13) == 0) {
                fprintf(stdout, "Exista deja un user conectat cu acest ID\n");
                break;
            } else if (strncmp(buf, ";MONEY_SENT;", 12) == 0) {
                fprintf(stdout, "Banii au fost trimisi cu success\n");
            } else if (strncmp(buf, ";MONEY_NOSENT;", 14) == 0) {
                fprintf(stdout, "Nu exista userul pentru care ai vrut sa trimiti banii\n");
            } else  if (strncmp(buf, ";SHOW_SUM", 9) == 0) {
                char junk[100];
                int sum = 0;
                sscanf(buf, "%s %d", junk, &sum);
                printf("Suma din cont este: %d\n", sum);
            } else if (strncmp(buf, ";GET_ERROR;", 11) == 0) {
                fprintf(stdout, "Suma de bani din cont este prea mica\n");
            } else if (strncmp(buf, ";GET_MONEY", 10) == 0) {
                char junk[100];
                int sum = 0;
                sscanf(buf, "%s %d", junk, &sum);
                printf("Ai primit suma: %d\n", sum);
            } else if (strncmp(buf, ";EXIT;", 6) == 0) {
                break;
            }
        }
    
        if (FD_ISSET(STDIN, &tmpFd)) { // comenzi de la stdin
            char buffNew[MAX_LEN];
            memset(buf, 0, MAX_LEN);
            fgets(buf, MAX_LEN - 1, stdin);
            buf[strcspn(buf, "\n")] = 0;
            if (strncmp(buf, "send_to", 7) == 0) {
                char id[50], junk[50], sumx[50];
                int sum = 0;
                sscanf(buf,"%s %s %s", junk, id, sumx);
                sum = atoi(sumx);
                if (strcmp(id, argv[1]) == 0) {
                    printf("Nu iti poti trimite bani tie\n");
                    continue;
                }
                if (sum == 0) {
                    printf("sum is not a number\n");
                } else {
                    sendto(sockUdp, (const char *)buf, strlen(buf), 0, (struct sockaddr*)NULL, sizeof(servAddr));
                }
            } else if (strncmp(buf, "show_sum", 8) == 0) {
                sprintf(buffNew, "show_sum %s", argv[1]);
                
                sendto(sockUdp, (const char *)buffNew, strlen(buffNew), 0, (struct sockaddr*)NULL, sizeof(servAddr));
            } else if (strncmp(buf, "get", 3) == 0) {
                char junk[50], sumx[50];
                int sum = 0;
                sscanf(buf,"%s %s", junk, sumx);
                sum = atoi(sumx);
                if (sum == 0) {
                    printf("sum is not a number\n");
                } else {
                    sprintf(buffNew, "%s %s", buf, argv[1]);
                    sendto(sockUdp, (const char *)buffNew, strlen(buffNew), 0, (struct sockaddr*)NULL, sizeof(servAddr));
                }
            } else if (strncmp(buf, "exit", 4) == 0) {
                sprintf(buffNew, "exit %s", argv[1]);
                sendto(sockUdp, (const char *)buffNew, strlen(buffNew), 0, (struct sockaddr*)NULL, sizeof(servAddr));
                break;
            } else {
                printf("Comanda nu exista\n");
            }
        }

    }

    close(sockUdp);
    return 0;
}