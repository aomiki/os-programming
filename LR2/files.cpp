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

bool get_file_info(std::string filepath, file_info& info_buff)
{
    struct stat buff;
    if(stat(filepath.c_str(), &buff) == -1)
    {
        return false;
    }

    info_buff.sizeBytes = buff.st_size;
    info_buff.permissions = buff.st_mode;
    info_buff.last_changed = buff.st_mtime;

    return true;
}

bool change_permissions(std::string filepath, mode_t mode)
{
    if(chmod(filepath.c_str(), mode) == -1)
    {
        return false;
    }

    return true;
}

bool copy_file(std::string src, std::string dest)
{
    int srcDesc = open(src.c_str(), O_RDONLY, 0);

    if (srcDesc == -1)
    {
        return false;
    }

    int destDesc = open(dest.c_str(), O_WRONLY | O_CREAT, 0644);

    if (destDesc == -1)
    {
        return false;
    }

    // good values should fit to blocksize, like 1024 or 4096
    char buf[BUFSIZ];
    size_t size;

    while ((size = read(srcDesc, buf, BUFSIZ)) > 0)
    {
        write(destDesc, buf, size);
    }

    close(srcDesc);
    close(destDesc);

    return true;
}

bool move_file(std::string src, std::string dest)
{
    if(rename(src.c_str(), dest.c_str()) == -1)
    {
        return false;
    }

    return true;
}
