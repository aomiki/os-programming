#include <string>

struct file_info {
    int sizeBytes;
    time_t last_changed;
    int permissions;
};

bool exists(std::string filepath);
file_info get_file_info(std::string filepath);
void copy_file(std::string src, std::string dest);
void move_file(std::string src, std::string dest);
void change_permissions(std::string filepath, mode_t mode);
