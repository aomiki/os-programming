#include <string>

enum CommandType: char
{
    Null = 0,
    ResetColor = 1,
    SetColor = 2
};

const CommandType min_command_num = ResetColor;
const CommandType max_command_num = SetColor;

const std::string pipe_name = "/tmp/colorpipe";