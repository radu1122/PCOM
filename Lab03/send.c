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
    msg t;
    pkt p;

    int inputFile, bufferSize;

	printf("[SENDER] Starting.\n");	
	init(HOST, PORT);
    inputFile = open(SEND_FILE_NAME, O_RDONLY);

	printf("[SENDER]: BDP=%d\n", atoi(argv[1]));
	
	for (int i = 0; i < COUNT; i++) {
		/* cleanup msg */
		memset(&t, 0, sizeof(msg));
		lseek(inputFile, 0, SEEK_SET);


		bufferSize = read(inputFile, p.payload, MSGSIZE - 5);
		
		if (bufferSize < 0) {
            perror("[SENDER] Unable to read from input file\n");
        } else {
			t.len = bufferSize;
			p.parity = 0;

			for (int q = 0; q < bufferSize; q++) {
                for (int j = 0; j < BITSNR; j++) {
                    p.parity ^= (1 << j) & p.payload[q];
                }
            }

			memcpy(t.payload, p.payload, MSGSIZE);

            printf("[SENDER] Sending payload: %s\n", t.payload);

            send_message(&t);
            recv_message(&t);
			if (strcmp(t.payload, "ACK") == 0) {
				printf("[SENDER] Got reply ACK\n");
			} else {
				printf("[SENDER] Receiver didn't send ACK\n");
			}
		}
	}

	printf("[SENDER] Job done, all sent.\n");
	close(inputFile);

	return 0;
}
