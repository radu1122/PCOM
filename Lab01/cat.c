#include<stdio.h>
#include<stdlib.h> 
#include<fcntl.h>
#include<unistd.h>

int main(int argc,char *argv[])
{ 
	int inputFile, buffer; 
	
		
		inputFile = open(argv[1],O_RDONLY);
		
		if(inputFile < 0) { 
			perror("open"); 
			exit(-1);
		} 
		
		while(read(inputFile, & buffer, 1)) {
			write(STDOUT_FILENO, & buffer, 1);

        }
		
		close(inputFile);

	return 0;
}