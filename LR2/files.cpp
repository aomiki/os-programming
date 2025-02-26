#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include "files.h"

bool exists(std::string filepath)
{
    struct stat buff;
    return stat(filepath.c_str(), &buff) == 0;
}

file_info get_file_info(std::string filepath)
{
    struct stat buff;
    stat(filepath.c_str(), &buff);

    file_info info;
    info.sizeBytes = buff.st_size;
    info.permissions = buff.st_mode;
    info.last_changed = buff.st_mtime;    

    return info;
}

void change_permissions(std::string filepath, mode_t mode)
{
    chmod(filepath.c_str(), mode);
}

void copy_file(std::string src, std::string dest)
{
    int srcDesc = open(src.c_str(), O_RDONLY, 0);
    int destDesc = open(dest.c_str(), O_WRONLY | O_CREAT, 0644);

    // good values should fit to blocksize, like 1024 or 4096
    char buf[BUFSIZ];
    size_t size;

    while ((size = read(srcDesc, buf, BUFSIZ)) > 0)
    {
        write(destDesc, buf, size);
    }

    close(srcDesc);
    close(destDesc);
}

void move_file(std::string src, std::string dest)
{
    rename(src.c_str(), dest.c_str());
}
