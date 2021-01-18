#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "tarball.h"

#define LENGTH(arr) (sizeof(arr) / sizeof(arr[0]))

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)

const char NUMS[] = "0123456789"; 

const int FILE_TYPES[] = {
    S_IFREG,    // '0' regular file   
    0,          // '1' hard links 
    S_IFLNK,    // '2' symbolic link 
    S_IFCHR,    // '3' character device 
    S_IFBLK,    // '4' block device 
    S_IFDIR,    // '5' directory  
    S_IFIFO,    // '6' FIFO
}; 

unsigned short checksum(char header[512])
{
    unsigned short c = 0; 
    for(int i = 0; i < 512; ++i)
        c += header[i]; 
    return c; 
}

char ftype(int mode)
{
    int type = mode & S_IFMT; 
    for(int i = 0; i < (int) LENGTH(FILE_TYPES); ++i)
        if(type == FILE_TYPES[i])
            return NUMS[i]; 
    return 0; 
}

void populate(const char *field, char header[512], int start, int size)
{
    int n = 0; 
    while(n < size && field[n])
    {
        header[start+n] = field[n]; 
        ++n; 
    }
    while(n < size)
    {
        header[start+n] = 0; 
        ++n; 
    }
}

int get_header(const char *path, char header[512]) 
{
    struct stat buf; 
    int status = stat(path, &buf); 

    char strfmt[20]; 
    if(status >= 0)
    {
        // filename
        int pathlen = strlen(path); 
        int pathbeg = pathlen > 99 ? pathlen - 99 : 0;  
        populate(path, header + pathbeg, 0, 100); 

        // permissions
        sprintf(strfmt, "%07o", buf.st_mode & 0777); 
        populate(strfmt, header, 100, 8); 

        // numerical uid and gid
        sprintf(strfmt, "%07o", buf.st_uid); 
        populate(strfmt, header, 108, 8); 
        sprintf(strfmt, "%07o", buf.st_gid); 
        populate(strfmt, header, 116, 8); 

        // size
        sprintf(strfmt, "%011o", buf.st_size); 
        populate(strfmt, header, 124, 12); 

        // last modified time
        sprintf(strfmt, "%011o", buf.st_mtim.tv_sec); 
        populate(strfmt, header, 136, 12); 

        // checksum calculated later
        populate("       ", header, 148, 7);  

        // file type
        char ft = buf.st_nlink == 1 ? ftype(buf.st_mode) : '1'; 
        populate(" ", header, 155, 1);
        populate(&ft, header, 156, 1); 

        // symbolic links
        char lnk[100]; 
        int bytes = readlink(path, lnk, LENGTH(lnk)-1); 
        lnk[bytes] = 0; 
        populate(lnk, header, 157, 100); 

        // ustar indicator
        populate("ustar  ", header, 257, 8); 

        // user and group name
        char user[32]; 
        strncpy(user, getpwuid(buf.st_uid)->pw_name, LENGTH(user)-1); 
        populate(user, header, 265, 32); 
        char group[32]; 
        strncpy(group, getgrgid(buf.st_gid)->gr_name, LENGTH(group)-1); 
        populate(group, header, 297, 32); 

        // major and minor devices
        if(S_ISCHR(buf.st_mode))
        {
            sprintf(strfmt, "%07lu", buf.st_rdev);    // major
            populate(strfmt, header, 329, 8); 
            sprintf(strfmt, "%07lu", buf.st_rdev);    // minor
            populate(strfmt, header, 337, 8); 
        }
        else 
        {
            populate("", header, 329, 8); 
            populate("", header, 337, 8); 
        }

        // filename prefix
        char prefix[155];   
        strncpy(prefix, path, MIN(pathbeg, (int) LENGTH(prefix)-1)); 
        populate(prefix, header, 345, 155); 

        // padding bytes
        populate("", header, 500, 12); 

        // calculate checksum
        sprintf(strfmt, "%06o", checksum(header)); 
        populate(strfmt, header, 148, 7); 
    }

    return status; 
}

void archive(const char *out, char *filesv[])
{
    int td = open(out, O_CREAT | O_WRONLY, 0644); 
    int fd; 
    char block[512]; 
    int b; 

    while(*filesv)
    {
        // header
        get_header(*filesv, block);  
        write(td, block, LENGTH(block)); 

        // content
        fd = open(*filesv, O_RDONLY); 
        while((b = read(fd, block, LENGTH(block))) == LENGTH(block))
            write(td, block, LENGTH(block)); 
        populate("", block, b, LENGTH(block)-b); 
        write(td, block, LENGTH(block)); 
        close(fd); 

        ++filesv; 
    }

    close(td); 
}
