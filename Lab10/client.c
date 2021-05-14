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
    char *message;
    char *response;
    char *host_api = "34.118.48.238";
    char *host_weather = "api.openweathermap.org";
    char *apiKeyCookie = NULL, *weatherKeyCookie = NULL;
    char **body_data, **cookies;
    int sockfd;

    // init 10x50 body_data, cookies arrays of strings    
    body_data=calloc(10, sizeof(char*));

    for(int i = 0; i < 10; ++i) {
        body_data[i] = calloc(50, sizeof(char));
    }

    cookies=calloc(10, sizeof(char*));

    for(int i = 0; i < 10; ++i) {
        cookies[i] = calloc(500, sizeof(char));
    }

    sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);

        
    // Ex 1.1: GET dummy from main server
    message = compute_get_request(host_api, "/api/v1/dummy", NULL, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s\n\n", response);
    free(message);
    free(response);

    // Ex 1.2: POST dummy and print response from main server
    strcpy(body_data[0], "key1=value1");
    message = compute_post_request(host_api, "/api/v1/dummy", "application/x-www-form-urlencoded",
             body_data, 1, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s\n\n", response);
    free(message);
    free(response);

    // Ex 2: Login into main server
    // get api key
    strcpy(body_data[0], "username=student");
    strcpy(body_data[1], "password=student");
    message = compute_post_request(host_api, "/api/v1/auth/login", "application/x-www-form-urlencoded",
             body_data, 2, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s\n\n", response);

    char* start_cookie = strstr(response, "connect.sid");
    apiKeyCookie = calloc(100, sizeof(char));
    memcpy(apiKeyCookie, start_cookie, 96);

    free(message);
    free(response);

    // already logged in
    strcpy(body_data[0], "username=student");
    strcpy(body_data[1], "password=student");
    strcpy(cookies[0], apiKeyCookie);
    message = compute_post_request(host_api, "/api/v1/auth/login", "application/x-www-form-urlencoded",
             body_data, 2, cookies, 1);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s\n\n", response);

    if(apiKeyCookie == NULL) {
            char* start_cookie = strstr(response, "connect.sid");
            apiKeyCookie = calloc(100, sizeof(char));
            memcpy(apiKeyCookie, start_cookie, 96);
    }

    free(message);
    free(response);

    // Ex 3: GET weather key from main server
    strcpy(cookies[0], apiKeyCookie);
    message = compute_get_request(host_api, "/api/v1/weather/key", NULL, cookies, 1);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s\n\n", response);

    // extract weather key
    weatherKeyCookie = basic_extract_json_response(response) + 8;
    weatherKeyCookie[strlen(weatherKeyCookie) - 2] = '\0';

    free(message);
    free(response);

    // Ex 4: GET weather data from OpenWeather API
    int openweatherfd = open_connection("37.139.20.5", 80, AF_INET, SOCK_STREAM, 0);

    char query_params[100] = {0};
    sprintf(query_params, "lat=45&lon=23&appid=%s", weatherKeyCookie);
    message = compute_get_request(host_weather, "/data/2.5/weather", query_params, NULL, 0);
    send_to_server(openweatherfd, message);
    response = receive_from_server(openweatherfd);
    printf("%s\n\n", response);
    free(message);
    free(response);

    // Ex 5: POST weather data for verification to main server
    // Ex 6: Logout from main server
    message = compute_get_request(host_api, "/api/v1/auth/logout", NULL, cookies, 1);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s\n\n", response);
    free(message);
    free(response);

    // BONUS: make the main server return "Already logged in!"

    // free the allocated data at the end!
    for(int i = 0; i < 10; ++i) {
        free(body_data[i]);
        free(cookies[i]);
    }

    free(body_data);
    free(cookies);
    free(apiKeyCookie);
    return 0;
}
