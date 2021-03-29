#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10001
#define SEND_FILE_NAME "received.txt"
#define RESPONSE_STR "ACK"


int main(){
  msg r, s;
  init(HOST,PORT);

    int outputFile, outputFileSize;

    char inputFileName[MAX_LEN];

    memset(r.payload, 0, MAX_LEN);
    memset(s.payload, 0, MAX_LEN);

    if (recv_message(&r) < 0){
        perror("Receive message");
        return -1;
    }

    printf("[receive] Got msg with payload: %s\n", r.payload);

    sscanf(r.payload, "%s %d" , inputFileName, &outputFileSize);

    outputFile = open(SEND_FILE_NAME, O_WRONLY | O_CREAT, 0777);

    strcpy(s.payload, RESPONSE_STR);
    s.len = strlen(s.payload) + 1;
    send_message(&s);

    while (outputFileSize) {
        if (recv_message(&r) < 0) {
            perror("Receive message");
            return -1;
        }
        printf("[receive] Got msg with payload: %s\n", r.payload);
        write(outputFile, r.payload, r.len);
        send_message(&s);
        outputFileSize = outputFileSize - r.len;
        
    }

    close(outputFile);

    return 0;
}
