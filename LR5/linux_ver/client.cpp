#include <iostream>
#include <stdio.h>
#include <signal.h>
#include <atomic>
#include <semaphore.h>
#include <string>
#include <random>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include "shared_objects.h"

std::atomic<bool> isInterrupted = false;

void handleInterrupt(int s)
{
    isInterrupted = true;
}

void waitForUserInput(int client_num)
{	
	std::cout << std::endl << "[client " << client_num << "] " << "<Enter anything to exit>" << std::endl;
	char _;
	std::cin >> _;
}

ShredingerStatus next_shredinger_status(ShredingerStatus current)
{
	std::random_device rd;
	std::mt19937 mt(rd());

	switch (current)
	{
		case ShredingerStatus::Null:
		{
			std::uniform_int_distribution<unsigned char> dist(min_shredinger_status, max_shredinger_status);
		
			return (ShredingerStatus)dist(mt);	
			break;
		}
		case ShredingerStatus::Undefined:
		{
			std::uniform_int_distribution<unsigned char> dist(Alive, Dead);
		
			return (ShredingerStatus)dist(mt);	
			break;
		}
		default:
			return current;
	}
}

int main(int argc, char *argv[])
{
	int ret_status;

    signal(SIGINT, handleInterrupt);

	if (argc <= 1)
	{
		std::cout << "Wrong number of program arguments" << std::endl;
		waitForUserInput(-1);
		return -1;
	}

	char client_num = std::stoi(argv[1]);
	ShredingerStatus curr_status = ShredingerStatus::Null;

	sem_t* sem_handle = sem_open(semaphore_name.c_str(), O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH, 0);

	if(sem_handle == SEM_FAILED)
	{
		printf("[client %d] failed to open semaphore (%d).\n", client_num, errno);
		waitForUserInput(client_num);
		return -1;
	}

	while (!isInterrupted && curr_status != ShredingerStatus::Dead)
	{
		std::cout << "[client " << +client_num << "] " << "Waiting for connection..." << std::endl;
		ret_status = sem_wait(sem_handle);

		if(ret_status == -1)
		{
			printf("[client %d] Wait for status write error (%d)\n", client_num, errno); 
			waitForUserInput(client_num);
			return -1;
		}

		curr_status = next_shredinger_status(curr_status);

		std::cout << "[client " << +client_num << "] " << "Opening pipe..." << std::endl;
		//Connect to pipe
		int pipe_fd = open(pipe_name.c_str(), O_WRONLY);
		if (pipe_fd == -1)
		{
			printf("[client %d] opening pipe failed (%d).\n", client_num, errno);
			return -1;
		}

		std::cout << "[client " << +client_num << "] " << "Connected to pipe" << std::endl;

		std::cout << "[client " << +client_num << "] " << "Reporting status: " << curr_status << std::endl;

		//Write to the pipe
		//1st byte - client number, 2nd byte - shredinger status
		unsigned char message[2];
		message[0] = client_num;
		message[1] = curr_status;
	
		if (isInterrupted) continue;

        ret_status = write(pipe_fd, message, sizeof(unsigned char) * 2);

        if (ret_status != sizeof(unsigned char) * 2)
        {
            if (errno == EPIPE)
            {
                std::cout << "[client " << +client_num << "] " << "Server closed" << std::endl;
            }
            else
            {
                printf("[client %d] writing to pipe failed (%d).\n", client_num, errno);
            }
        }

		ret_status = close(pipe_fd);

		if(ret_status == -1)
		{
			printf("[client %d] closing pipe failed (%d).\n", client_num, errno);
			return -1;
		}
	}

	waitForUserInput(client_num);

	ret_status = sem_close(sem_handle);

	if(ret_status == -1)
	{
		printf("[client %d] failed to close semaphore (%d).\n", client_num, errno);
		waitForUserInput(client_num);
		return -1;
	}
}
