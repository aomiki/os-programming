#include <iostream>
#include <bitset>
#include <algorithm>
#include "args_parser.h"

#include <cstdlib>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

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

int m_index(int x, int y, int width, int height)
{
    //assuming row-first data placement
    return width * y + x;
}

int sign(unsigned int* perms, int n)
{
    int cnt=0;
    for(int i=0;i<n;i++)
        for(int j=i+1;j<n;j++)
            if (perms[i]>perms[j]) cnt++;

    return cnt%2;
}


double det(double* matrix, int width, int height)
{
    if (width != height)
    {
        return 0;
    }
    
    double sum = 0;

    unsigned int* cols = new unsigned int[width];
    for (size_t x = 0; x < width; x++) cols[x] = x;

    do
    {
        double multipl = 1;
        for (size_t y = 0; y < height; y++)
        {
            multipl *= matrix[m_index(cols[y], y, width, height)];
        }

        if (sign(cols, width)==1)
        {
            multipl *= -1;
        }
        

        sum += multipl;
    } while (std::next_permutation(cols, cols+width));
    
    return sum;
}


std::vector<double> inverse(double* matrix, int width, int height)
{
    
}

void front()
{
    int width, height;

    std::cout << "Enter matrix width and height" << std::endl;
    std::cin >> width >> height;

    double* matrix = new double[width * height];
    for (size_t y = 0; y < height; y++)
    {
        std::cout << "Enter row " << y << ":" << std::endl;
        for (size_t x = 0; x < width; x++)
        {
            std::cin >> matrix[m_index(x, y, width, height)];
        }

        std::cout << std::endl;
    }

    std::cout << "resulting matrix" << std::endl;
    for (size_t y = 0; y < height; y++)
    {
        for (size_t x = 0; x < width; x++)
        {
            std::cout << matrix[m_index(x,y, width, height)] << " ";
        }
        std::cout << std::endl;
    }

    double d = det(matrix, width, height);

    std::cout << "determinant: " << d << std::endl;
}

void back()
{

}

/// @brief Variant 7
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
        return 1;
    }

    if (args.command == help)
    {
        print_help(std::cout);
    }

    pid_t pid;

    pid = fork();

    switch (pid) {
    case -1:
        perror("fork");
        exit(EXIT_FAILURE);
    case 0:
        back();
        puts("Child exiting.");
        fflush(stdout);
        _exit(EXIT_SUCCESS);
    default:
        printf("Child is PID %jd\n", (intmax_t) pid);
        front();
        exit(EXIT_SUCCESS);
    }    
}
