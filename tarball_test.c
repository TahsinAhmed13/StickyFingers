#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "tarball.h"

int main(int argsc, char *argsv[])
{
    struct tarball *tar; 

    if(argsc >= 4 && !strcmp(argsv[1], "-c"))
    {
        char *out = argsv[2]; 

        int f = argsc - 3; 
        char **filesv = (char **) malloc(sizeof(char *) * (f+1)); 
        for(int i = 0; i < f; ++i)
            filesv[i] = argsv[i+3]; 
        filesv[f] = NULL; 
        tar = new_tarball(out, filesv); 
        free(filesv); 
        
        double p; 
        while((p = archive(tar, 20)) < 1)
        {
            printf("%.2f%%\n", p * 100); 
            sleep(1); 
        }
        del_tarball(tar); 
    }
    else if(argsc >= 3 && !strcmp(argsv[1], "-x"))
    {
        char *out = argsv[2]; 
        char *filesv[] = {NULL}; 
        tar = new_tarball(out, filesv); 

        double p; 
        while((p = extract(tar, 20)) < 1)
        {
            printf("%.2f%%\n", p * 100); 
            sleep(1); 
        }
    }
    else 
        printf("Invalid arguments!\n"); 

    return 0; 
}
