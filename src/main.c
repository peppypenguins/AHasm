#include "main.h"

int main(){
char inputFilePath[64];
char outputFilePath[64]; 

obtainFilePath(inputFilePath, outputFilePath, 64);

printf("Entered input file path, max length 64: %s\n",inputFilePath);
printf("Entered output file path, max length 64: %s\n", outputFilePath);

assemble(inputFilePath,outputFilePath);
printf("Successfully assembled given program");
    return 0;
}