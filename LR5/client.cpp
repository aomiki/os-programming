#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <conio.h>
#include <atomic>
#include <string>
#include <random>
#include "shared_objects.h"

DWORD originalAttr = 0;

std::atomic<bool> isInterrupted = false;

BOOL WINAPI handleInterrupt(DWORD id)
{
    isInterrupted = true;
    return TRUE;
}

void waitForUserInput(int client_num)
{	
	std::cout << std::endl << "[client " << client_num << "] " << "<Press ENTER to exit>" << std::endl;
	_getch();
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
    SetConsoleCtrlHandler(handleInterrupt, TRUE);

	if (argc <= 1)
	{
		std::cout << "Wrong number of program arguments" << std::endl;
		waitForUserInput(-1);
		return -1;
	}
	
	char client_num = std::stoi(argv[1]);
	ShredingerStatus curr_status = ShredingerStatus::Null;

	BOOL fSuccess = FALSE;
	DWORD cbRead, dwMode;
	LPTSTR pipename = TEXT("\\\\.\\pipe\\shredingerpipe");

	//1. try to get past mutex

	HANDLE startEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, TEXT("Local\\ShredingerStart"));
    HANDLE mutex = CreateMutex(
        NULL,
        FALSE,
        TEXT("Local\\ShredingerStartMutex")
    );

	if (mutex == NULL)
	{
		printf("[client %d] Create mutex error (%d)\n", client_num, GetLastError()); 
		waitForUserInput(client_num);
		return -1;
	}

	while (!isInterrupted && curr_status != ShredingerStatus::Dead)
	{
		std::cout << "[client " << +client_num << "] " << "Waiting for connection..." << std::endl;
		DWORD mutexStart = WaitForSingleObject( 
			mutex, // event handle
			INFINITE);    // indefinite wait

		if(mutexStart != WAIT_OBJECT_0)
		{
			printf("[client %d] Wait for status write error (%d)\n", client_num, GetLastError()); 
			waitForUserInput(client_num);
			return -1; 
		}

		std::cout << "[client " << +client_num << "] " << "First in line, waiting for connection..." << std::endl;

		//0. wait for start
		DWORD eventStartWaitRes = WaitForSingleObject( 
			startEvent, // event handle
			INFINITE);    // indefinite wait
	
		if (eventStartWaitRes != WAIT_OBJECT_0)
		{
			printf("[client %d] Wait for start error (%d)\n", client_num, GetLastError()); 
			waitForUserInput(client_num);
			return -1; 
		}

		std::cout << "[client " << +client_num << "] " << "Connected!" << std::endl;

		curr_status = next_shredinger_status(curr_status);

		HANDLE pipe;
		//Connect to pipe
		while (1)
		{
			pipe = CreateFile(
				pipename,  // pipe name
				GENERIC_READ | // read and write access
					GENERIC_WRITE,
				0,			   // no sharing
				NULL,		   // default security attributes
				OPEN_EXISTING, // opens existing pipe
				0,			   // default attributes
				NULL);		   // no template file

			// Break if the pipe handle is valid.
			if (pipe != INVALID_HANDLE_VALUE)
				break;

			// Exit if an error other than ERROR_PIPE_BUSY occurs.
			if (GetLastError() != ERROR_PIPE_BUSY)
			{
				std::cout << "[client " << +client_num << "] " << "Could not open pipe. GLE=" <<  GetLastError() << std::endl;
				waitForUserInput(client_num);
				return -1;
			}

			// All pipe instances are busy, so wait for 20 seconds.
			if (!WaitNamedPipe(pipename, 20000))
			{
				printf("[client %d] Could not open pipe: 20 second wait timed out.", client_num);
				waitForUserInput(client_num);
				return -1;
			}
		}

		std::cout << "[client " << +client_num << "] " << "Connected to pipe" << std::endl;

		std::cout << "[client " << +client_num << "] " << "Reporting status: " << curr_status << std::endl;

		//Write to the pipe
		//1st byte - client number, 2nd byte - shredinger status
		unsigned char message[2];
		message[0] = client_num;
		message[1] = curr_status;
	
		if (isInterrupted) continue;

		DWORD dwRead;
		if (!WriteFile(pipe, message, sizeof(unsigned char) * 2, &dwRead, NULL))
		{
			printf("[client %d] Send error. (%d)\n", client_num, GetLastError());
		}

		CloseHandle(pipe);

		// Release ownership of the mutex object
		if (!ReleaseMutex(mutex)) 
		{
			printf("[client %d] Error releasing mutex. (%d)\n", client_num, GetLastError());
			waitForUserInput(client_num);
			return -1;
		}
	}

	waitForUserInput(client_num);
}
