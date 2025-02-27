#include <iostream>
#include <bitset>
#include "args_parser.h"
#include "files.h"

void bad_args(std::string msg = "")
{
    std::cout << "bad arguments";

    if (msg != "")
    {
        std::cout << " (" << msg << ")";
    }

    std::cout << std::endl;
}

void runtime_err(std::string msg = "")
{
    std::cout << "runtime error";

    if (msg != "")
    {
        std::cout << " (" << msg << ")";
    }

    std::cout << std::endl;
}

std::string command_to_string(program_command cmd)
{
    switch (cmd)
    {
        case help:
            return "help";
        case info:
            return "info";
        case move:
            return "move";
        case copy:
            return "copy";
        case chmod:
            return "chmod";
        case none:
            return "none";
        default:
            return "ehm";
    }
}

void debug_print_args(program_arguments args)
{
    std::cout << "program_arguments:" << std::endl;
    std::cout << "  command: " << command_to_string(args.command) << std::endl;
    for (size_t i = 0; i < args.pos_args.size(); i++)
    {
        std::cout << "  posarg" << i << ": " << args.pos_args[i] << std::endl;
    }
}

int exec_copy(program_arguments args)
{
    std::string src = args.pos_args[0];
    std::string dest = args.pos_args[1];

    if (!exists(src))
    {
        bad_args("src doesn't exist");
        return 1;
    }

    if (exists(dest))
    {
        bad_args("dest exists");
        return 1;
    }

    if(!copy_file(src, dest))
    {
        runtime_err("access error");
        return 1;
    }

    return 0;
}

int exec_move(program_arguments args)
{
    std::string src = args.pos_args[0];
    std::string dest = args.pos_args[1];

    if (!exists(src))
    {
        bad_args("src doesn't exist");
        return 1;
    }

    if (exists(dest))
    {
        bad_args("dest exists");
        return 1;
    }

    if(!move_file(src, dest))
    {
        runtime_err("access error");
        return 1;
    }

    return 0;
}

int exec_info(program_arguments args)
{
    std::string file = args.pos_args[0];

    if (!exists(file))
    {
        bad_args("file doesn't exist");
        return 1;
    }

    file_info info;
    if(!get_file_info(file, info))
    {
        runtime_err("access error");
        return 1;
    }

    std::cout << "Size: " << info.sizeBytes << " Bytes" << std::endl;
    std::cout << "Permissions: " << std::oct << info.permissions << " (" << std::bitset<9>(info.permissions) << ")" << std::endl;
    std::cout << "Last modified time: " << ctime(&info.last_changed) << std::endl;

    return 0;
}

int exec_chmod(program_arguments args)
{
    std::string file = args.pos_args[0];

    if (!exists(file))
    {
        bad_args("file doesn't exist");
        return 1;
    }

    int mode = std::stoi(args.pos_args[1], nullptr, 2);

    if(!change_permissions(file, mode))
    {
        bad_args("access error");
        return 1;
    }

    return 0;
}

/// @brief 11. Написать функцию вычисления среднего значения 
/// элементов заданного одномерного массива.
/// @param argc Number of command-line arguments
/// @param argv Command-line arguments, where argv[0] is name of the program.
/// @return 
int main(int argc, char const *argv[])
{
    program_arguments args;
    parse_error err =  parse_args(argc, argv, args);
    debug_print_args(args);

    if (err.is_error)
    {
        bad_args(err.err_msg);
    }

    if (args.command == help)
    {
        print_help(std::cout);
    }

    if (args.command == chmod)
    {
        exec_chmod(args);
    }

    if (args.command == info)
    {
        exec_info(args);
    }
    
    if (args.command == copy)
    {
        return exec_copy(args);
    }

    if (args.command == move)
    {
        return exec_move(args);
    }
}
