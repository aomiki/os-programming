#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <atomic>
#include "shared_objects.h"

std::atomic<bool> isInterrupted = false;

void handleInterrupt(int id)
{
    isInterrupted = true;
}

int main()
{
    signal(SIGINT, handleInterrupt);
    signal(SIGPIPE, handleInterrupt);

    //Create pipe
    int ret_status = mkfifo(pipe_name.c_str(), S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);

    if (ret_status == -1 && errno != EEXIST)
    {
        std::cout << "error creating named pipe: " << errno << std::endl;
    }

    std::cout << "Creating client..." << std::endl;
    ret_status = system("alacritty --title client -e ./client.out &");
    if(ret_status == -1)
    {
        printf("client creation failed (%d).\n", errno);
        return -1;
    }

    std::cout << "Waiting for connection" << std::endl;
    // wait for someone to connect to the pipe
    int pipe_fd = open(pipe_name.c_str(), O_WRONLY);

    if (pipe_fd == -1)
    {
        printf("opening pipe failed (%d).\n", errno);
        return -1;
    }

    while (!isInterrupted)
    {
        CommandType clientCommand = Null;
        std::cout << "Choose action:" << std::endl;
        std::cout << "0. Exit" << std::endl;
        std::cout << "1. Reset" << std::endl;
        std::cout << "2. Set color" << std::endl;

        unsigned user_choice;
        std::cin >> user_choice;

        if(user_choice == 0)
        {
            isInterrupted = true;
            continue;
        }

        if (user_choice < min_command_num || user_choice > max_command_num)
            continue;
    
        clientCommand = (CommandType)user_choice;

        unsigned red, green, blue;
        if (clientCommand == SetColor)
        {
            std::cout << "Enter color:" << std::endl;
    
            std::cout << "red: ";
            std::cin >> red;
            std::cout << std::endl;
            if(red > 255) continue;
    
            std::cout << "green: ";
            std::cin >> green;
            std::cout << std::endl;
            if(green > 255) continue;
    
            std::cout << "blue: ";
            std::cin >> blue;
            std::cout << std::endl;
            if(blue > 255) continue;
        }

        if (isInterrupted) continue;

        unsigned char msg[4];
        msg[0] = clientCommand;
        msg[1] = red;
        msg[2] = green;
        msg[3] = blue;

        ret_status = write(pipe_fd, msg, sizeof(unsigned char) * 4);

        if (ret_status != sizeof(unsigned char) * 4)
        {
            if (errno == EPIPE)
            {
                std::cout << "Client closed" << std::endl;
            }
            else
            {
                printf("writing to pipe failed (%d).\n", errno);
            }
        }
    }

    ret_status = close(pipe_fd);

    if(ret_status == -1)
    {
        printf("closing pipe failed (%dc).\n", errno);
        return -1;
    }

    std::cout << "Connection closed" << std::endl;
}
