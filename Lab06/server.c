/*
*  	Protocoale de comunicatii: 
*  	Laborator 6: UDP
*	mini-server de backup fisiere
*/

#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "helpers.h"


void usage(char*file)
{
	fprintf(stderr,"Usage: %s server_port file\n",file);
	exit(0);
}

/*
*	Utilizare: ./server server_port nume_fisier
*/
int main(int argc,char**argv)
{
	int fd;

	if (argc!=3)
		usage(argv[0]);
	
	struct sockaddr_in my_sockaddr, from_station ;
	char buf[BUFLEN];


	/*Deschidere socket*/
	int sockid = socket(PF_INET, SOCK_DGRAM, 0);
 
	if (sockid == -1) {
		perror("[ERROR] Failed to open socket \n");
	}

	
	/*Setare struct sockaddr_in pentru a asculta pe portul respectiv */
	struct sockaddr_in sockIn;

	sockIn.sin_family = AF_INET;
	sockIn.sin_port = htons((uint16_t)atoi(argv[1])); 
	sockIn.sin_addr.s_addr = INADDR_ANY;
	
	/* Legare proprietati de socket */

	int r = bind(sockid, (struct sockaddr*) &sockIn, sizeof(sockIn));

	if(r < 0){
		perror("[ERROR] Failed to bind to socket.\n");
	}
	
	/* Deschidere fisier pentru scriere */
	DIE((fd=open(argv[2],O_WRONLY|O_CREAT|O_TRUNC,0644))==-1,"open file");
	
	/*
	*  cat_timp  mai_pot_citi
	*		citeste din socket
	*		pune in fisier
	*/

	struct sockaddr_in client;
	socklen_t length = sizeof(client);

	while((r = recvfrom(sockid, buf, BUFLEN, 0, (struct sockaddr*) &client, &length)) != -1) {

		if(strncmp(buf, "DONE", 4) == 0) {
			break;
		}
		write(fd, buf, r);
		sleep(1);
	} 


	/*Inchidere socket*/	
	close(sockid);

	/*Inchidere fisier*/
	close(fd);

	return 0;
}
