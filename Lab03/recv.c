#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

typedef struct {
    char payload[1396];
    int parity;
} pkt;

int main(void)
{
	msg r;
	pkt p;

	printf("[RECEIVER] Starting.\n");
	init(HOST, PORT);
	
	for (int i = 0; i < COUNT; i++) {
		memset(r.payload, 0, MSGSIZE);
		int checkBit = 0;
		
		if (recv_message(&r) < 0) {
			perror("[RECEIVER] Receive error. Exiting.\n");
			return -1;
		}
		memcpy(p.payload, r.payload, MSGSIZE);

		for (int q = 0; q < r.len; q++) {
            for (int j = 0; j < 8; j++) {
                checkBit ^= (1 << j) & p.payload[q];
            }
        }

		memset(r.payload, 0, MSGSIZE);

		if (p.parity != checkBit) {
            printf("[RECEIVER] Pary Check not passed\n");
            sprintf(r.payload, "NOTACK");

        } else {
            printf("[RECEIVER Pary Check passed\n");
            sprintf(r.payload, "ACK");
        }

		/* send ACK */
		if (send_message(&r) < 0) {
			perror("[RECEIVER] Send ACK error. Exiting.\n");
			return -1;
		}
	}

	printf("[RECEIVER] Finished receiving..\n");
	return 0;
}
