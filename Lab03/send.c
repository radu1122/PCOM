#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000
#define SEND_FILE_NAME "sender.txt"
#define BITSNR 8

typedef struct {
    char payload[1396];
    int parity;
} pkt;

int main(int argc, char *argv[])
{
    msg t, p;

    int inputFile, bufferSize, df = 100;

	printf("[SENDER] Starting.\n");	
	init(HOST, PORT);
    inputFile = open(SEND_FILE_NAME, O_RDONLY);
	lseek(inputFile, 0, SEEK_SET);

	printf("[SENDER]: BDP=%d\n", atoi(argv[1]));
	df = 5;

	lseek(inputFile, 0, SEEK_CUR);

	for (int q = 0; q < COUNT; q++) {
        for(int i = 0; i < df; i++) {
            memset(&t, 0, sizeof(msg));

            bufferSize = read(inputFile, t.payload, 4);
            t.len = bufferSize;

            if (send_message(&t) < 0) {
                perror("[SENDER] Send error. Exiting.\n");
                return -1;
            }
            printf("[SENDER] SENT MESSAGE %s\n", t.payload);
        }

        for(int i = 0; i < 5; i++) {
            /* wait for ACK */
            if (recv_message(&p) < 0) {
                perror("[SENDER] Receive error. Exiting.\n");
                return -1;
            }
            int parity = 0;
            for (int q = 0; q < strlen(p.payload); q++) {
                for (int j = 0; j < BITSNR; j++) {
                    parity ^= (1 << j) & p.payload[q];
                }
            }

            memcpy(p.payload, t.payload, strlen(p.payload));
            t.len = strlen(p.payload);
            if (parity == p.len) {
                printf("[SENDER] Got reply correct ACK\n");
            } else {
                printf("[SENDER] Receiver didn't send correct ACK\n");
                send_message(&t);
                printf("[SENDER] SENT MESSAGE %d\n", i);
            }

        }
    }

	printf("[SENDER] Job done, all sent.\n");
	close(inputFile);

	return 0;
}
