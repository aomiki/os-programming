#include <iostream>
#include <errno.h>
#include <atomic>
#include <string>
#include <semaphore.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
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

    //Create pipepipe_name

    int ret_status = mkfifo(pipe_name.c_str(), S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);

    if (ret_status == -1 && errno != EEXIST)
    {
        std::cout << "error creating named pipe: " << errno << std::endl;
    }

    unsigned n_clients = 0;

    std::cout << "Enter the number of clients: " << std::endl << "> ";
    std::cin >> n_clients;

    sem_unlink(semaphore_name.c_str()); //clear semaphore
	sem_t* sem_handle = sem_open(semaphore_name.c_str(), O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH, 0);

	if(sem_handle == SEM_FAILED)
	{
		printf("failed to open semaphore (%d).\n", errno);
		return -1;
	}

    for (size_t i = 0; i < n_clients; i++)
    {
        std::cout << "Creating client " << i << std::endl;

        ret_status = system(("alacritty --title client -e ./client.out " + std::to_string(i) + "&").c_str());
        if(ret_status == -1)
        {
            printf("client creation failed (%d).\n", errno);
            return -1;
        }
    }

    while (!isInterrupted)
    {
        std::cout << "<enter anything to read next>" << std::endl;
        char _;
        std::cin >> _;

        if (isInterrupted) continue;

		// Release ownership of the semaphore object
		if (sem_post(sem_handle) == -1)
		{
			printf("error releasing semaphore. (%d)\n", errno);
			return -1;
		}

        std::cout << "Waiting for connection" << std::endl;

        // wait for someone to connect to the pipe
        int pipe_fd = open(pipe_name.c_str(), O_RDONLY);
    
        if (pipe_fd == -1)
        {
            printf("opening pipe failed (%d).\n", errno);
            return -1;
        }

        // Read from the pipe.
        unsigned char message[2];

        ret_status = read(pipe_fd, message, sizeof(unsigned char) * 2);

        if(isInterrupted) continue;

        if (ret_status != sizeof(unsigned char) * 2)
        {
            if (ret_status == -1)
            {
                printf("reading from pipe failed (%d).\n", errno);
            }
            else
            {
                std::cout << "client disconnected" << std::endl;
            }

            isInterrupted = true;
            continue; 
        }

        ret_status = close(pipe_fd);

        if(ret_status == -1)
        {
            printf("closing pipe failed (%dc).\n", errno);
            return -1;
        }

        std::cout << "client : " << +message[0] << " | reported status: " ;

        switch (message[1])
        {
            case ShredingerStatus::Null:
                std::cout << "null";
                break;
            case ShredingerStatus::Undefined:
                std::cout << "not defined";
                break;
            case ShredingerStatus::Alive:
                std::cout << "alive";
                break;
            case ShredingerStatus::Dead:
                std::cout << "dead";
                break;
            default:
                std::cout << "uhm";
                break;
        }

        std::cout << std::endl;

        std::cout << "Connection closed" << std::endl;
    }

	ret_status = sem_close(sem_handle);

	if(ret_status == -1)
	{
		printf("failed to close semaphore (%d).\n", errno);
		return -1;
	}
}
