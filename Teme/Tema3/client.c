#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

int main(int argc, char *argv[])
{
   
    char *response;
    int sockfd;


    sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);


    // Ex 1.2: POST dummy and print response from main server
    // char *message = "POST /api/v1/tema/auth/register HTTP/1.1\r\nHost: 34.118.48.238\r\nContent-Type: application/json\r\nContent-Length: 39\r\n\r\n{\"username\":\"t20est\",\"password\":\"test\"}\r\n";
    // send_to_server(sockfd, message);
    // response = receive_from_server(sockfd);
    // printf("%s%s\n\n", message, response);

    // char *ret;

    // ret = strstr(response, "error");
    // if (ret)
    //     printf("found substring at address %p\n", ret);
    // else
    //     printf("no substring found!\n");


    while (1) {
        char input[200];
        fgets(input, 200, stdin);
        printf("%s\n", input);

        if (strncmp(input, "exit", 4) == 0) {
            return 0;
        }
    }


    return 0;
}
