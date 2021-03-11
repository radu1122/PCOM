#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10000
#define SEND_FILE_NAME "send.txt"

int main(){
  init(HOST,PORT);
  msg t;

    int inputFile, inputFileSize, bufferSize;

    inputFile = open(SEND_FILE_NAME, O_RDONLY);
    inputFileSize = lseek(inputFile, 0, SEEK_END);
    lseek(inputFile, 0, SEEK_SET);

    snprintf(t.payload, 12, "%s %d", SEND_FILE_NAME, inputFileSize);

    t.len = strlen(t.payload) + 1;
    send_message(&t);


    if (recv_message(&t) < 0){
        perror("receive error");
    } else {
      if (strcmp(t.payload, "ACK") == 0) {
        printf("[send] Got reply ACK\n");
      } else {
        printf("[send] Receiver didn't send ACK\n");
      }
    }

    while ((bufferSize = read(inputFile, t.payload, MAX_LEN - 1))) {
        if (bufferSize < 0) {
            perror("Unable to read from input file\n");
        } else {
            t.len = bufferSize;
            send_message(&t);

            if (recv_message(&t) < 0){
                perror("receive error");
            }
            else {
                if (strcmp(t.payload, "ACK") == 0) {
                  printf("[send] Got reply ACK\n");
                } else {
                  printf("[send] Receiver didn't send ACK\n");
                }
            }

            memset(t.payload, 0, MAX_LEN);
        }
    }

    close(inputFile);

    return 0;
}
