#include <map>
#include "args_parser.h"
#include <cstring>
#include <iomanip>

struct cmd_params {

    bool optional = false;
    int num_pos_args = 0;
    std::vector<std::string> names;
    std::vector<std::string> pos_args_names;
    std::string help_page;
};

std::map<program_command, cmd_params> cmdToParams = 
{
    { help, { true, 0, { "--help", "-h" }, {}, "shows help page" } },
    { file, { true, 0, { "--file", "-f" }, {}, "read input from file and write output to file" } }
};

bool compare_command_str(std::vector<std::string> expected, const char* str)
{
    for (size_t i = 0; i < expected.size(); i++)
    {
        if (std::strcmp(expected[i].c_str(), str) == 0)
        {
            return true;
        }
    }

    return false;
}

bool check_main_command(const char* arg, program_command& command, bool only_optional = false)
{
    for (const auto &[c, params] : cmdToParams)
    {
        if (only_optional && !params.optional) continue;

        if(compare_command_str(params.names, arg))
        {
            command = c;
            return true;
        }
    }

    return false;
}

parse_error parse_args(int argc, char const *argv[], program_arguments& result)
{
    parse_error err;

    int i = 1;

    if (i >= argc)
    {
        return err;
    }

    if (check_main_command(argv[i], result.command))
    {
        i++;
    }

    if (result.command == help)
    {
        return err;
    }

    for (; i < argc; i++)
    {
        program_command cmd = none;
        if (check_main_command(argv[i], cmd, true))
        {
            if (cmd == help)
            {
                result.command = cmd;
                result.pos_args.clear();
                break;
            }
        }

        result.pos_args.push_back(argv[i]);
    }

    if (result.command == none)
    {
        err.is_error = true;
        err.err_msg = "no commands recognized";
        return err;
    }

    cmd_params cmdParams = cmdToParams[result.command];
    if (result.pos_args.size() != cmdParams.num_pos_args)
    {
        err.is_error = true;
        err.err_msg = "number of arguments should be " + std::to_string(cmdParams.num_pos_args);
    }

    return err;
}

std::string print_help(std::ostream& out)
{
    std::string result;
    for (const auto &[c, params] : cmdToParams)
    {
        for (size_t i = 0; i < params.names.size(); i++)
        {
            if (i != 0)
            {
                out << "|";
            }

            out << params.names[i];            
        }

        out << " " << std::setw(45) << std::left;

        std::string pos_args = "";
        for (size_t i = 0; i < params.num_pos_args; i++)
        {
            if (i != 0)
            {
                pos_args += " ";
            }

            pos_args += "[" + params.pos_args_names[i] + "]";
        }

        out << pos_args;

        out <<  std::left << params.help_page;

        out << "\n";
    }

    return result;
}