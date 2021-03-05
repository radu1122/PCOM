#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char * argv[]) {
    FILE * inputFile;
    char * line = NULL;
    size_t len = 0;
    char lines[1024][1024];


    if(argc != 2) { 
        printf("bad arguments\n");
        return -1;
    }

    inputFile = fopen(argv[1], "r");
    if (inputFile == NULL) {
        printf("Error Reading the fime\n");
        return -1;
    }


    int n = 0;
    while ((getline(&line, &len, inputFile)) != -1) {
        int lenStr = strlen(line);
        
        if (lenStr > 0 && line[lenStr - 1] == '\n') {
            line[lenStr - 1] = 0;
        }

        strcpy(lines[n], line);
        n++;
    }

    for(int i = n - 1; i >= 0; i--) {
        printf("%s\n", lines[i]);
    }

    fclose(inputFile);
    if (line) {
        free(line);
    }
    
    return 0;
}