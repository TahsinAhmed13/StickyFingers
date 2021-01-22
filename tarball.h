#include <sys/types.h>

struct tarball
{
    char *out; 
    char **filesv; 
    int fcur; 
    off_t pos; 
    off_t size;
    off_t done; 
}; 

struct tarball * new_tarball(char *, char **); 

double archive(struct tarball *, int); 
