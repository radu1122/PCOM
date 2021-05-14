// Protocoale de comunicatii
// Laborator 9 - DNS
// dns.c

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

int usage(char* name)
{
	printf("Usage:\n\t%s -n <NAME>\n\t%s -a <IP>\n", name, name);
	return 1;
}

// Receives a name and prints IP addresses
void get_ip(char* name)
{
	int ret;
	struct addrinfo hints, *result, *p;

	// TODO: set hints
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	hints.ai_flags = AI_CANONNAME;

	// TODO: get addresses
	char *service = "80";
	ret = getaddrinfo(name, service, &hints, &result);
	if (ret != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		exit(EXIT_FAILURE);
	}

	// TODO: iterate through addresses and print them
	char buf[100];

	for (p = result; p; p = p->ai_next) {
		if(p->ai_family == AF_INET) {
			struct sockaddr_in* entry = (struct sockaddr_in*) p->ai_addr;
			inet_ntop(AF_INET, &entry->sin_addr, buf, sizeof(buf));
			printf("%s. Port: %d. Canon name: %s. Protocol: %d\n", buf, entry->sin_port, p->ai_canonname, p->ai_protocol);
		} else {
			struct sockaddr_in6* entry = (struct sockaddr_in6*) p->ai_addr;
			inet_ntop(AF_INET6, &entry->sin6_addr, buf, sizeof(buf));
			printf("%s. Port: %d. Canon name: %s. Protocol: %d\n", buf, entry->sin6_port, p->ai_canonname, p->ai_protocol);
		}		
	}

	// TODO: free allocated data
	freeaddrinfo(result);
}

// Receives an address and prints the associated name and service
void get_name(char* ip)
{
	int ret;
	struct sockaddr_in addr;
	char host[1024];
	char service[20];

	// TODO: fill in address data
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &addr.sin_addr);
	addr.sin_port = 0;

	// TODO: get name and service
	ret = getnameinfo((struct sockaddr*) &addr, sizeof(struct sockaddr_in), host, sizeof(host),
						service,sizeof(service), 0);

	if(ret != 0) {
		fprintf(stderr, "Reverse DNS Lookup Failed: %s", gai_strerror(ret));
		exit(EXIT_FAILURE);
	}

	// TODO: print name and service
	printf("Host: %s. Service : %s\n", host, service);
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		return usage(argv[0]);
	}

	if (strncmp(argv[1], "-n", 2) == 0) {
		get_ip(argv[2]);
	} else if (strncmp(argv[1], "-a", 2) == 0) {
		get_name(argv[2]);
	} else {
		return usage(argv[0]);
	}

	return 0;
}
