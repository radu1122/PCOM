#include <stdio.h>
#include <stdlib.h>

int main(int argc, char * argv[]) {
    FILE * inputFile;
    char * line = NULL;
    size_t len = 0;

    if(argc != 2) { 
        printf("bad arguments\n");
        return -1;
    }

    inputFile = fopen(argv[1], "r");
    if (inputFile == NULL) {
        printf("Error Reading the fime\n");
        return -1;
    }



    while ((getline(&line, &len, inputFile)) != -1) {
        printf("%s", line);
    }

    fclose(inputFile);
    printf("\n");
    if (line) {
        free(line);
    }
    
    return 0;
}