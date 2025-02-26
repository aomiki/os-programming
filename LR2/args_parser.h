#include <string>
#include <vector>
#include <ostream>

struct parse_error 
{
    bool is_error = false;
    std::string err_msg = "";
};

enum program_command {
    none = 0,
    help = 1,
    copy = 2,
    move = 3,
    info = 4,
    chmod = 5,
};

struct program_arguments {
    program_command command = none;
    std::vector<std::string> pos_args;
};

parse_error parse_args(int argc, char const *argv[], program_arguments& result);

std::string print_help(std::ostream& out);
