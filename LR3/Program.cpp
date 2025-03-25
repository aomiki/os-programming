#include <iostream>
#include <bitset>
#include <algorithm>
#include "args_parser.h"

#include <err.h>
#include <cstring>
#include <cstdlib>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <iosfwd>
#include <iostream>
#include <fstream>
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
        case file:
            return "file";
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

    //Leibniz formula
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

void get_submatrix(double* submat_buff, double* matrix, unsigned width, unsigned height, unsigned x, unsigned y)
{
    unsigned j = 0;
    for (unsigned i = 0; i < height; i++)
    {
        if (i == y) continue;
        
        unsigned mat_start_i = width * i;
        unsigned sub_start_i = (width-1) * j;

        //copy first half of the row
        memcpy(
            submat_buff + sub_start_i,
            matrix + mat_start_i,
            x * sizeof(double)
        );

        //copy second half of the row
        memcpy(
            submat_buff + sub_start_i + x,
            matrix + mat_start_i + (x + 1),
            (width - (x + 1)) * sizeof(double)
        );

        j++;
    }
}

void print_matrix(double* matrix, unsigned width, unsigned height, std::ofstream& out)
{
    for (size_t y = 0; y < height; y++)
    {
        for (size_t x = 0; x < width; x++)
        {
            out << matrix[m_index(x,y, width, height)] << " ";
        }
        out << std::endl;
    }
}


double* inverse(double* matrix, int width, int height)
{
    if (width != height)
    {
       return nullptr; 
    }
    
    double d = det(matrix, width, height);

    if (d == 0)
    {
        return nullptr;
    }

    double* algebraic_mat = new double[width * height];
    for (size_t y = 0; y < height; y++)
    {
        for (size_t x = 0; x < width; x++)
        {
            double sign = 1.0;
            if (y % 2 == 0)
            {
                if (x % 2 != 0)
                    sign = -1.0;
            }
            else
            {
                if (x % 2 == 0)
                    sign = -1.0;
            }

            double* minor_submat = new double[(width-1)*(height-1)];
            get_submatrix(minor_submat, matrix, width, height, x, y);
            algebraic_mat[m_index(x, y, width, height)] = sign * det(minor_submat, width-1, height-1);

            delete [] minor_submat;
        }
    }

    //std::cout << "algebraic mat" << std::endl;
    //print_matrix(algebraic_mat, width, height);

    double* inv_mat = new double[width * height];
    for (size_t y = 0; y < height; y++)
    {
        for (size_t x = 0; x < width; x++)
        {
            inv_mat[m_index(x, y, width, height)]  = algebraic_mat[m_index(y, x , width, height)];
        }
    }

    //std::cout << "transposed algebraic mat" << std::endl;
    //print_matrix(inv_mat, width, height);

    for (size_t y = 0; y < height; y++)
    {
        for (size_t x = 0; x < width; x++)
        {
            inv_mat[m_index(x, y, width, height)]  = inv_mat[m_index(x, y , width, height)] / d;
        }
    }

    delete [] algebraic_mat;

    return inv_mat;
}

void front(int read_pipefd[2], int write_pipefd[2], std::string inp_file, std::string out_file, std::string prompts_file)
{
    std::ifstream in(inp_file);
    std::ofstream prompt(prompts_file);
    std::ofstream out(out_file);

    if (close(read_pipefd[1]) == -1)  /* Close unused write end */
        err(EXIT_FAILURE, "close");

    if (close(write_pipefd[0]) == -1)  /* Close unused read end */
        err(EXIT_FAILURE, "close");

    unsigned width, height;

    prompt << "Enter matrix width and height" << std::endl;
    in >> width >> height;

    double* matrix = new double[width * height];
    for (size_t y = 0; y < height; y++)
    {
        prompt << "Enter row " << y << ":" << std::endl;
        for (size_t x = 0; x < width; x++)
        {
            in >> matrix[m_index(x, y, width, height)];
        }

        prompt << std::endl;
    }

    prompt << "input matrix" << std::endl;
    print_matrix(matrix, width, height, out);

    //SEND WIDTH
    if (write(write_pipefd[1], &width, sizeof(unsigned)) != sizeof(unsigned))
        err(EXIT_FAILURE, "write");

    //SEND HEIGHT
    if (write(write_pipefd[1], &height, sizeof(unsigned)) != sizeof(unsigned))
        err(EXIT_FAILURE, "write");

    //SEND INPUT MATRIX
    if (write(write_pipefd[1], matrix, width * height * sizeof(double)) != (width * height * sizeof(double)) )
        err(EXIT_FAILURE, "write");

    //READ DETERMINANT
    double d = 0;
    if (read(read_pipefd[0], &d, sizeof(double)) != sizeof(double))
        err(EXIT_FAILURE, "read");

    prompt << "determinant: ";
    out << d << std::endl;

    //READ INVERSE MATRIX
    double* inv = new double[width * height];
    if (read(read_pipefd[0], inv, width * height * sizeof(double)) != (width * height * sizeof(double)) )
        err(EXIT_FAILURE, "read");

    prompt << "inverse: " << std::endl;
    print_matrix(inv, width, height, out);

    delete [] inv;
    delete [] matrix;
}

void back(int read_pipefd[2], int write_pipefd[2])
{
    if (close(read_pipefd[1]) == -1)  /* Close unused write end */
        err(EXIT_FAILURE, "close");

    if (close(write_pipefd[0]) == -1)  /* Close unused read end */
        err(EXIT_FAILURE, "close");

    unsigned width, height;

    //READ WIDTH
    if (read(read_pipefd[0], &width, sizeof(unsigned)) != sizeof(unsigned))
        err(EXIT_FAILURE, "read");
    
    //READ HEIGHT
    if (read(read_pipefd[0], &height, sizeof(unsigned)) != sizeof(unsigned))
        err(EXIT_FAILURE, "read");

    //READ INPUT MATRIX
    double* matrix = new double[width * height];
    if (read(read_pipefd[0], matrix, width * height * sizeof(double)) != (width * height * sizeof(double)))
        err(EXIT_FAILURE, "read");

    double d = det(matrix, width, height);

    //SEND DETERMINANT
    if (write(write_pipefd[1], &d, sizeof(double)) != sizeof(double))
        err(EXIT_FAILURE, "write");

    double* inv = inverse(matrix, width, height);

    //SEND INVERSE MATRIX
    if (write(write_pipefd[1], inv, width * height * sizeof(double)) != (width * height * sizeof(double)))
        err(EXIT_FAILURE, "write");

    delete [] inv;
    delete [] matrix; 
}

/// @brief Variant 7
/// @param argc Number of command-line arguments
/// @param argv Command-line arguments, where argv[0] is name of the program.
/// @return 
int main(int argc, char const *argv[])
{
    program_arguments args;
    parse_error perr =  parse_args(argc, argv, args);
    debug_print_args(args);

    if (perr.is_error)
    {
        bad_args(perr.err_msg);
        return 1;
    }

    if (args.command == help)
    {
        print_help(std::cout);
        return 0;
    }

    std::string inp_filename = "/dev/stdin", prompts_filename = "/dev/stdout", out_filename = "/dev/stdout";
    if (args.command == file)
    {
        inp_filename = "input";
        out_filename = "output";
        prompts_filename = "/dev/null";
    }

    int back_to_front_pipefd[2];
    int front_to_back_pipefd[2];

    if (pipe(back_to_front_pipefd) == -1)
        err(EXIT_FAILURE, "pipe");

    if (pipe(front_to_back_pipefd) == -1)
        err(EXIT_FAILURE, "pipe");

    pid_t pid = fork();

    switch (pid) {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
        case 0:
        {
            back(front_to_back_pipefd, back_to_front_pipefd);
            puts("Child (backend) exiting.");
            if (close(front_to_back_pipefd[0]) == -1)  /* Close read end */
                err(EXIT_FAILURE, "close");
        
            if (close(back_to_front_pipefd[1]) == -1)  /* Close write end */
                err(EXIT_FAILURE, "close");
            fflush(stdout);
            _exit(EXIT_SUCCESS);
        }
        default:
        {
            printf("Child is PID %jd\n", (intmax_t) pid);
            front(back_to_front_pipefd, front_to_back_pipefd, inp_filename, out_filename, prompts_filename);

            if (close(back_to_front_pipefd[0]) == -1)  /* Close read end */
                err(EXIT_FAILURE, "close");
        
            if (close(front_to_back_pipefd[1]) == -1)  /* Close write end */
                err(EXIT_FAILURE, "close");

            exit(EXIT_SUCCESS);
        }
    }    
}
