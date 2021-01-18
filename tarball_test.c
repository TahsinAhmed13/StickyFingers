#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tarball.h"

int main(int argsc, char *argsv[])
{
    if(argsc < 4)
    {
        printf("Not enough arguments!\n"); 
        exit(0); 
    }
    else if(strcmp(argsv[1], "-o"))
    {
        printf("No output file!\n"); 
        exit(0); 
    }

    char *out = argsv[2]; 
    int f = argsc - 3; 
    char **filesv = (char **) malloc(sizeof(char *) * (f+1)); 
    for(int i = 0; i < f; ++i)
        filesv[i] = argsv[i+3]; 
    filesv[f] = NULL; 

    archive(out, filesv); 
    free(filesv); 

    return 0; 
}
