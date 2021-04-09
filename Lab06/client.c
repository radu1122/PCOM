/*
*  	Protocoale de comunicatii: 
*  	Laborator 6: UDP
*	client mini-server de backup fisiere
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
	fprintf(stderr,"Usage: %s ip_server port_server file\n",file);
	exit(0);
}

/*
*	Utilizare: ./client ip_server port_server nume_fisier_trimis
*/
int main(int argc,char**argv)
{
	if (argc!=4)
		usage(argv[0]);
	
	int fd;
	struct sockaddr_in to_station;
	char buf[BUFLEN];


	/*Deschidere socket*/
	
	int sockid = socket(PF_INET, SOCK_DGRAM, 0);
 
	if (sockid == -1) {
		perror("[ERROR] failed to open socket\n");
	}
	
	/* Deschidere fisier pentru citire */
	DIE((fd=open(argv[3],O_RDONLY))==-1,"open file");
	
	/*Setare struct sockaddr_in pentru a specifica unde trimit datele*/

	struct sockaddr_in sockIn;

	sockIn.sin_family = AF_INET;
	sockIn.sin_port = htons((uint16_t)atoi(argv[2])); 
	inet_aton(argv[1], &sockIn.sin_addr);	
	
	
	/*
	*  cat_timp  mai_pot_citi
	*		citeste din fisier
	*		trimite pe socket
	*/	

	int readerFile;

	while ((readerFile = read(fd, buf, BUFLEN))) {
        if (readerFile < 0)
            perror("Can't read from file \n");

        int bytes_sent = sendto(sockid, buf, readerFile, 0, (struct sockaddr*) &sockIn, sizeof(sockIn));

		if(bytes_sent == -1) {
			perror("Send error \n");
		} else {
			printf("[Client] Trimitere cu success");
		}

		//sleep(1);
    }

	strcpy(buf, "DONE");

	int bytes_sent = sendto(sockid, buf, strlen(buf) + 1, 0, (struct sockaddr*) &sockIn, sizeof(sockIn));

	if(bytes_sent == -1) {
		perror("[Client] Trimiterea mesajului nu s-a putut termina cu success\n");
	} else {
		printf("[Client] Tot mesajul a fost trimis \n");

	}

	/*Inchidere socket*/
	close(sockid);
	
	/*Inchidere fisier*/
	close(fd);

	return 0;
}
