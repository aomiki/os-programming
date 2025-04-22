#include <iostream>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include "shared_objects.h"
#include <atomic>
#include <unistd.h>
#include <string.h>
#include <sstream>


std::string default_osc_color = "";

void setBgColor(unsigned char r, unsigned char g, unsigned char b)
{
	std::ostringstream str;
	str << "\x1b]11;rgb:" << std::hex << +r << "/" << std::hex << +g << "/" << std::hex << +b << "\x07";

	const std::string bg_color = str.str();
    write(STDOUT_FILENO, bg_color.c_str(), strlen(bg_color.c_str()));
}

void resetColor()
{
    const char* reset_bg = "\x1b]11;rgb:1818/1818/1818\x1b\\";
    write(STDOUT_FILENO, reset_bg, strlen(reset_bg));
}

std::atomic<bool> isInterrupted = false;

void handleInterrupt(int s)
{
    isInterrupted = true;
}

int main()
{
	int ret_status;

    signal(SIGINT, handleInterrupt);

	std::cout << "Waiting for connection..." << std::endl;
	
	int pipe_fd = open(pipe_name.c_str(), O_RDONLY);
	if (pipe_fd == -1)
	{
		printf("opening pipe failed (%d).\n", errno);
		return -1;
	}

	std::cout << "Connection established" << std::endl;

	while (!isInterrupted)
	{
		// Read from the pipe.
		unsigned char msg[4];

		ret_status = read(pipe_fd, msg, sizeof(unsigned char) * 4);

		if(isInterrupted) continue;

        if (ret_status != sizeof(unsigned char) * 4)
        {
			if (ret_status == -1)
			{
				printf("reading from pipe failed (%d).\n", errno);
			}
			else
			{
				std::cout << "Server disconnected" << std::endl;
			}

			isInterrupted = true;
			continue; 
        }

		CommandType command = (CommandType)msg[0];

		switch (command)
		{
			case CommandType::SetColor:
				std::cout << "Accepted color: (" << +msg[1] << ", " << +msg[2] << ", " << +msg[3] << ")" << std::endl;
				setBgColor(msg[1], msg[2], msg[3]);
				break;

			case CommandType::ResetColor:
				std::cout << "Resetting color." << std::endl;
				resetColor();
				break;
			default:
				std::cout << "Invalid command" << std::endl;
				break;
		}
	}

	std::cout << std::endl << "<Press ENTER to terminate connection and exit>" << std::endl;

	char _;
	std::cin >> _;

    ret_status = close(pipe_fd);

    if(ret_status == -1)
    {
        printf("closing pipe failed (%d).\n", errno);
        return -1;
    }

    std::cout << "Connection closed" << std::endl;
}
