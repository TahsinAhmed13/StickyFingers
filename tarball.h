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
void del_tarball(struct tarball *); 

double progress(const struct tarball *); 

double archive(struct tarball *, int); 
double extract(struct tarball *, int); 
