#include <string>

struct file_info {
    int sizeBytes;
    time_t last_changed;
    unsigned permissions;
};

bool exists(std::string filepath);
bool get_file_info(std::string filepath, file_info& info_buff);
bool copy_file(std::string src, std::string dest);
bool move_file(std::string src, std::string dest);
bool change_permissions(std::string filepath, mode_t mode);
