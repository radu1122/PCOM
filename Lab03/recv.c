#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "lib.h"
#define BITSNR 8

#define HOST "127.0.0.1"
#define PORT 10001

int main(void)
{
	msg r;
	
	printf("[RECEIVER] Starting.\n");
	init(HOST, PORT);
	
	for (int i = 0; i < COUNT; i++) {
		/* wait for message */
		if (recv_message(&r) < 0) {
			perror("[RECEIVER] Receive error. Exiting.\n");
			return -1;
		}

		int parity = 0;
		for (int q = 0; q < strlen(r.payload); q++) {
			for (int j = 0; j < BITSNR; j++) {
				parity ^= (1 << j) & r.payload[q];
			}
		}
		r.len = parity;
		/* send  ACK */
		if (send_message(&r) < 0) {
			perror("[RECEIVER] Send ACK error. Exiting.\n");
			return -1;
		}
	}

	printf("[RECEIVER] Finished receiving..\n");
	return 0;
}