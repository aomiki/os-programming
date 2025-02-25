#include <vector>
#include <iostream>
#include <string>

void bad_args(std::string msg = "")
{
    std::cout << "bad arguments";

    if (msg != "")
    {
        std::cout << " (" << msg << ")";
    }

    std::cout << std::endl;
}

/// @brief 11. Написать функцию вычисления среднего значения 
/// элементов заданного одномерного массива.
/// @param argc Number of command-line arguments
/// @param argv Command-line arguments, where argv[0] is name of the program.
/// @return 
int main(int argc, char const *argv[])
{
    int arg_num = argc-1;
    if (arg_num == 0)
    {
        bad_args("no arguments");
        return 1;
    }

    int nums[arg_num];

    for (int i = 1; i < argc; i++)
    {
        try
        {
            nums[i-1] = std::stod(argv[i]);
        }
        catch(const std::exception& e)
        {
            bad_args(e.what());
            return 1;
        }
    }

    double sum = 0;
    for (int i = 0; i < arg_num; i++)
    {
        sum += nums[i];
    }

    double result = sum / arg_num;

    std::cout << result << std::endl;
    
    return 0;
}
