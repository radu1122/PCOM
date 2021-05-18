#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"

int main(int argc, char *argv[])
{
   
    char *response;
    int sockfd;
    char cookie[200];
    char jwt[300];
    int jwtAccess = 0;
    int loggedIn = 0;

    char *ret;

    printf("Commands list: register, login, enter_library, get_books, get_book, add_book, delete_book, logout, exit\n");

    while (1) {
        char input[200];
        fgets(input, 200, stdin);

        if (strncmp(input, "exit", 4) == 0) {
            return 0;
        } else if (strncmp(input, "register", 8) == 0) {
            sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
            char name[200], pass[200];
            printf("username=");
            fgets(name, 200, stdin);
            name[strcspn(name, "\n")] = 0;
            printf("password=");
            fgets(pass, 200, stdin);
            pass[strcspn(pass, "\n")] = 0;

            char message[500];
            int len = 29 + strlen(pass) + strlen(name);
            sprintf(message, "POST /api/v1/tema/auth/register HTTP/1.1\r\nHost: 34.118.48.238\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n{\"username\":\"%s\",\"password\":\"%s\"}\r\n", len, name, pass);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            ret = strstr(response, "error");
            if (ret) {
                printf("The username is already taken\n");
            }
            else {
                printf("Register with success\n");
            }
            close(sockfd);
        } else if (strncmp(input, "login", 5) == 0) {
            sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
            char name[200], pass[200];
            printf("username=");
            fgets(name, 200, stdin);
            name[strcspn(name, "\n")] = 0;
            printf("password=");
            fgets(pass, 200, stdin);
            pass[strcspn(pass, "\n")] = 0;

            char message[500];
            int len = 29 + strlen(pass) + strlen(name);
            sprintf(message, "POST /api/v1/tema/auth/login HTTP/1.1\r\nHost: 34.118.48.238\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n{\"username\":\"%s\",\"password\":\"%s\"}\r\n", len, name, pass);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            ret = strstr(response, "error");
            if (ret) {
                printf("Credentialele sunt gresite");
            }
            else {
                char* start_cookie = strstr(response, "connect.sid");
                if(start_cookie == NULL)
                    continue;
                char* end_cookie = strstr(response, "Date:");
                *(end_cookie - 20) = '\0';
                strncpy(cookie, start_cookie, 200);
                loggedIn = 1;
                printf("Login with success\n");
            }
            close(sockfd);
        } else if (strncmp(input, "enter_library", 13) == 0) {
            if (loggedIn != 1) {
                printf("Nu esti conectat\n");
                continue;
            }
            char message[500];
            sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
            
            sprintf(message, "GET /api/v1/tema/library/access HTTP/1.1\r\nHost: 34.118.48.238\r\nCookie: %s\r\n\r\n", cookie);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            ret = strstr(response, "error");
            if (ret) {
                printf("Unkown Error");
            }
            else {
                char* end_token = strstr(response, "\"}");
                *(end_token) = '\0';
                char* start_token = strstr(response, "{\"token\":");
                start_token = start_token + 10;
                strncpy(jwt, start_token, 300);
                jwtAccess = 1;
                printf("Enter library wtih success\n");
            }
            close(sockfd);
        } else if (strncmp(input, "get_books", 9) == 0) {
            if (loggedIn != 1 || jwtAccess != 1) {
                printf("Nu esti conectat la librarie\n");
                continue;
            }
            sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);

            char message[500];
            sprintf(message, "GET /api/v1/tema/library/books HTTP/1.1\r\nHost: 34.118.48.238\r\nAuthorization: Bearer %s\r\nCookie: %s\r\n\r\n", jwt, cookie);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            ret = strstr(response, "error");
            if (ret) {
                printf("Unkown Error\n");
            }
            else {
                char* start_json = strstr(response, "[{\"");
                printf("%s\n", start_json);
            }
            close(sockfd);
        } else if (strncmp(input, "get_book", 8) == 0) {
            if (loggedIn != 1 || jwtAccess != 1) {
                printf("Nu esti conectat la librarie\n");
                continue;
            }

            char id[200];
            printf("id=");
            fgets(id, 200, stdin);
            id[strcspn(id, "\n")] = 0;

            sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);

            char message[500];
            sprintf(message, "GET /api/v1/tema/library/books/%s HTTP/1.1\r\nHost: 34.118.48.238\r\nAuthorization: Bearer %s\r\nCookie: %s\r\n\r\n",id, jwt, cookie);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            ret = strstr(response, "error");
            if (ret) {
                printf("Cartea cu acest ID nu exista\n");
            }
            else {
                char* start_json = strstr(response, "[{\"");
                printf("%s\n", start_json);
            }
            close(sockfd);
        }  else if (strncmp(input, "add_book", 8) == 0) {
            if (loggedIn != 1 || jwtAccess != 1) {
                printf("Nu esti conectat la librarie\n");
                continue;
            }

            sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
            char title[200], author[200], genre[200], publisher[200], page_count[200];

            printf("title=");
            fgets(title, 200, stdin);
            title[strcspn(title, "\n")] = 0;

            printf("author=");
            fgets(author, 200, stdin);
            author[strcspn(author, "\n")] = 0;

            printf("genre=");
            fgets(genre, 200, stdin);
            genre[strcspn(genre, "\n")] = 0;

            printf("publisher=");
            fgets(publisher, 200, stdin);
            publisher[strcspn(publisher, "\n")] = 0;

            printf("page_count=");
            fgets(page_count, 200, stdin);
            page_count[strcspn(page_count, "\n")] = 0;

            char message[500];
            int len = 21 + 45 + strlen(title) + strlen(author) + strlen(genre) + strlen(publisher) + strlen(page_count);
            char payload[500];
            sprintf(payload, "{\"title\":\"%s\",\"author\":\"%s\",\"genre\":\"%s\",\"publisher\":\"%s\",\"page_count\":%d}", title, author, genre, publisher, atoi(page_count));
            sprintf(message, "POST /api/v1/tema/library/books HTTP/1.1\r\nHost: 34.118.48.238\r\nContent-Type: application/json\r\nAuthorization: Bearer %s\r\nCookie: %s\r\nContent-Length: %d\r\n\r\n%s\r\n",jwt, cookie, len, payload);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            ret = strstr(response, "error");
            if (ret) {
                printf("Datele introduse sunt gresite");
            }
            else {
                printf("Book added with success\n");
            }
            close(sockfd);
        } else if (strncmp(input, "delete_book", 11) == 0) {
            if (loggedIn != 1 || jwtAccess != 1) {
                printf("Nu esti conectat la librarie\n");
                continue;
            }

            char id[200];
            printf("id=");
            fgets(id, 200, stdin);
            id[strcspn(id, "\n")] = 0;

            sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);

            char message[500];
            sprintf(message, "DELETE /api/v1/tema/library/books/%s HTTP/1.1\r\nHost: 34.118.48.238\r\nAuthorization: Bearer %s\r\nCookie: %s\r\n\r\n",id, jwt, cookie);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            ret = strstr(response, "error");
            if (ret) {
                printf("Cartea cu acest ID nu exista\n");
            }
            else {
                printf("Cartea a fost stearsa cu success\n");
            }
            close(sockfd);
        } else if (strncmp(input, "logout", 6) == 0) {
            if (loggedIn != 1 || jwtAccess != 1) {
                printf("Nu esti conectat\n");
                continue;
            }

            sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);

            char message[500];
            sprintf(message, "GET /api/v1/tema/auth/logout HTTP/1.1\r\nHost: 34.118.48.238\r\nAuthorization: Bearer %s\r\nCookie: %s\r\n\r\n", jwt, cookie);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            ret = strstr(response, "error");
            if (ret) {
                printf("Unkown error\n");
            }
            else {
                printf("Logout with success\n");
            }
            loggedIn = 0;
            jwtAccess = 0;
            close(sockfd);
        } else {
            printf("Comand unkown\n");
            printf("Commands list: register, login, enter_library, get_books, get_book, add_book, delete_book, logout, exit\n");
        }
    }


    return 0;
}
