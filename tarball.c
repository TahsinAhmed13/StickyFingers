#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>

#include "tarball.h"

#define RECORD_SIZE 512

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

unsigned short get_checksum(char *header)
{
    unsigned short c = 0; 
    for(int i = 0; i < RECORD_SIZE; ++i)
        c += header[i]; 
    return c; 
}

char get_ftype(int mode)
{
    int type = mode & S_IFMT; 
    for(int i = 0; i < (int) LENGTH(FILE_TYPES); ++i)
        if(type == FILE_TYPES[i])
            return NUMS[i]; 
    return 0; 
}

void populate(const char *field, char *header, int start, int size)
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

int get_header(const char *path, char *header) 
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
        char ft = buf.st_nlink == 1 ? get_ftype(buf.st_mode) : '1'; 
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
        sprintf(strfmt, "%06o", get_checksum(header)); 
        populate(strfmt, header, 148, 7); 
    }

    return status; 
}

void create_parent_dirs(char *path)
{
    char *dir = path; 
    while((path = strchr(path, '/')))
    {
        path[0] = 0; 
        mkdir(dir, 0755); 
        path[0] = '/'; 
        path++; 
    }
}

int create_file(char *header)
{
    // get checksum from header
    char checksum_copy[8]; 
    strcpy(checksum_copy, header + 148); 
    int checksum = strtol(header + 148, NULL, 8); 
    
    // calculate checksum
    populate("       ", header, 148, 7);  
    int mychecksum = get_checksum(header); 
    populate(checksum_copy, header, 148, 7); 

    // validate
    if(checksum != mychecksum) 
        return -1; 

    // congregate metadata
    char *filename = header; 
    int perms = strtol(header + 100, NULL, 8); 
    int ftype = atoi(header + 155); 
    char *lnkname = header + 157; 

    // create file with permissions
    create_parent_dirs(filename); 
    switch(ftype)
    {
        case 1: // hard link 
            link(lnkname, filename); 
            chmod(filename, perms); 
            break; 
        case 2: // soft link 
            symlink(lnkname, filename); 
            chmod(filename, perms); 
            break; 
        case 5: // directory 
            mkdir(filename, perms); 
            break; 
        case 6: // FIFO 
            mkfifo(filename, perms); 
            break; 
    }
    int fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, perms); 

    // change ownerships
    uid_t uid = strtol(header + 108, NULL, 8); 
    gid_t gid = strtol(header + 116, NULL, 8); 
    chown(filename, uid, gid); 

    return fd; 
}

int get_files_num(char **filesv)
{
    int n = 0; 
    while(*filesv)
    {
        n++; 
        filesv++; 
    }
    return n; 
}

off_t get_files_size(char **filesv)
{
    off_t size = 0; 
    struct stat buf; 
    while(*filesv)
    {
        stat(*filesv, &buf); 
        size += buf.st_size; 
        filesv++; 
    }
    return size; 
}

off_t get_tar_size(char *tarname)
{
    char header[RECORD_SIZE]; 
    off_t size = 0; 

    int td = open(tarname, O_RDONLY); 
    int end = lseek(td, 0, SEEK_END); 
    int cur = lseek(td, 0, SEEK_SET); 

    while(cur < end)
    {
        read(td, header, RECORD_SIZE); 
        int fsize = strtol(header + 124, NULL, 8); 
        size += fsize; 
        int bytes = RECORD_SIZE * ceil((double) fsize / RECORD_SIZE); 
        cur = lseek(td, bytes, SEEK_CUR); 
    }
    close(td); 

    return size;  
}

struct tarball * new_tarball(char *out, char **filesv)
{
    struct tarball *tar = (struct tarball *) malloc(sizeof(struct tarball)); 

    tar->out = (char *) malloc(sizeof(char) * (strlen(out)+1)); 
    strcpy(tar->out, out); 
    
    tar->filesv = (char **) malloc(sizeof(char *) * (get_files_num(filesv)+1)); 
    int i;
    for(i = 0; filesv[i]; ++i)
    {
        tar->filesv[i] = (char *) malloc(sizeof(char) * (strlen(filesv[i]+1))); 
        strcpy(tar->filesv[i], filesv[i]); 
    }
    tar->filesv[i] = NULL; 

    tar->fcur = 0; 
    tar->pos = 0; 
    if(tar->filesv[0])  tar->size = get_files_size(filesv); 
    else                tar->size = get_tar_size(out); 
    tar->done = 0; 

    return tar; 
}

void del_tarball(struct tarball *tar)
{
    free(tar->out); 
    for(int i = 0; tar->filesv[i]; ++i)
        free(tar->filesv[i]); 
    free(tar->filesv); 
    free(tar); 
}

double progress(const struct tarball *tar)
{
    return (double) tar->done / tar->size; 
}

double archive(struct tarball *tar, int records)
{
    char buffer[RECORD_SIZE * records]; 
    char *bufptr = buffer; 

    char *filename = tar->filesv[tar->fcur]; 
    while(filename && (bufptr - buffer) < LENGTH(buffer))
    {
        // open files
        int fd = open(filename, O_RDONLY); 
        lseek(fd, tar->pos, SEEK_SET);  

        // load header (if necessary)
        if(tar->pos == 0)
        {
            get_header(filename, bufptr); 
            bufptr += RECORD_SIZE; 
        }
        
        // load files contents
        int nbytes = LENGTH(buffer) - (bufptr - buffer); 
        int rbytes = read(fd, bufptr, nbytes);   
        bufptr += rbytes; 
        if(rbytes % RECORD_SIZE)
        {
            // uneven record aka file finished
            int padding = RECORD_SIZE - (rbytes % RECORD_SIZE); 
            populate("", bufptr, 0, padding); 
            bufptr += padding; 
        }
        tar->done += rbytes; 

        // go to next file
        tar->pos = lseek(fd, 0, SEEK_CUR); 
        if(tar->pos == lseek(fd, 0, SEEK_END))
        {
            tar->fcur++; 
            tar->pos = 0; 
            filename = tar->filesv[tar->fcur]; 
        }

        close(fd); 
    }
    // add padding
    int nbytes = LENGTH(buffer) - (bufptr - buffer); 
    populate("", bufptr, 0, nbytes); 

    // write to tar
    // TODO: truncate when file exists
    int td = open(tar->out, O_CREAT | O_WRONLY, 0644); 
    lseek(td, 0, SEEK_END); 
    write(td, buffer, LENGTH(buffer)); 
    close(td); 

    return progress(tar); 
}

double extract(struct tarball *tar, int records)
{
    char header[RECORD_SIZE]; 
    char buffer[RECORD_SIZE * records]; 
    
    int td = open(tar->out, O_RDONLY); 
    off_t end = lseek(td, 0, SEEK_END); 

    while(tar->pos < end && records > 0)
    {
        // read current file's header
        lseek(td, tar->pos, SEEK_SET); 
        read(td, header, RECORD_SIZE); 

        int fd; 
        if(!tar->fcur)
        {
            // create new file
            fd = create_file(header); 
            records--; 
            tar->fcur = 1; 
        }
        else 
            fd = open(header, O_WRONLY); 
        
        // set correct position in tar
        off_t cur_size = lseek(fd, 0, SEEK_END); 
        off_t total_size = strtol(header + 124, NULL, 8); 
        lseek(td, cur_size, SEEK_CUR); 
        
        // read from tar 
        int r = ceil((double) (total_size - cur_size) / RECORD_SIZE);  
        int nbytes = RECORD_SIZE * MIN(r, records);  
        int rbytes = read(td, buffer, nbytes); 
        int bytes = strlen(buffer); 

        // write to file
        write(fd, buffer, bytes); 
        records -= rbytes / RECORD_SIZE; 
        tar->done += bytes; 

        // go to next file
        if(total_size == cur_size + bytes)
        {
            tar->fcur = 0; 
            tar->pos = lseek(td, 0, SEEK_CUR); 
        }
        
        close(fd); 
    }

    close(td); 
    
    return progress(tar); 
}
