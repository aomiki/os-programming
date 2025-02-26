#include <iostream>
#include "args_parser.h"

void bad_args(std::string msg = "")
{
    std::cout << "bad arguments";

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
    
    
}
