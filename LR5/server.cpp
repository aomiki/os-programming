#include <windows.h>
#include <iostream>
#include <atomic>
#include <string>
#include <conio.h>
#include "shared_objects.h"

std::atomic<bool> isInterrupted = false;

BOOL WINAPI handleInterrupt(DWORD id)
{
    isInterrupted = true;
    return TRUE;
}

int main()
{
    SetConsoleCtrlHandler(handleInterrupt, TRUE);
    HANDLE pipe;

    //Create pipe
    pipe = CreateNamedPipe(
        TEXT("\\\\.\\pipe\\shredingerpipe"), PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES, 1, 1, 0, NULL);

    if (pipe == INVALID_HANDLE_VALUE)
    {
        std::cout << "error creating named pipe" << std::endl;
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );
 
    HANDLE startEvent = CreateEvent( 
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,              // initial state is nonsignaled
        TEXT("Local\\ShredingerStart")  // object name
    );

    if (startEvent == NULL) 
    { 
        printf("CreateEvent failed (%d)\n", GetLastError());
        return -1;
    }

    unsigned n_clients = 0;

    std::cout << "Enter the number of clients: " << std::endl << "> ";
    std::cin >> n_clients;

    for (size_t i = 0; i < n_clients; i++)
    {
        std::cout << "Creating client " << i << std::endl;

        // Start the child process. 
        if( !CreateProcess(TEXT("client.exe"),   // No module name (use command line)
            const_cast<char *>(("client.exe " + std::to_string(i)).c_str()),        // Command linej
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            CREATE_NEW_CONSOLE,  // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory 
            &si,            // Pointer to STARTUPINFO structure
            &pi )           // Pointer to PROCESS_INFORMATION structure
        )
        {
            printf( "CreateProcess failed (%d).\n", GetLastError() );
            return -1;
        }
    }

    if (pipe != INVALID_HANDLE_VALUE)
    {
        while (!isInterrupted)
        {   
            std::cout << "<press any key to read next>" << std::endl;
            _getch();

            if (isInterrupted) continue;

            if (!SetEvent(startEvent)) 
            {
                printf("SetEvent failed (%d)\n", GetLastError());
                return -1;
            }

            std::cout << "Waiting for connection" << std::endl;
            if (ConnectNamedPipe(pipe, NULL) == FALSE)   // wait for someone to connect to the pipe
            {
                printf("Pipe connection failed (%d).\n", GetLastError() );
                continue;
            }

            if (!ResetEvent(startEvent)) 
            {
                printf("ResetEvent failed (%d)\n", GetLastError());
                return -1;
            }

		    // Read from the pipe.
            unsigned char message[2];
            DWORD cbRead;
            BOOL fSuccess = ReadFile(
                pipe,					 // pipe handle
                message,					 // buffer to receive reply
                sizeof(unsigned char) * 2, // size of buffer
                &cbRead,				 // number of bytes read
                NULL);					 // not overlapped

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

            DisconnectNamedPipe(pipe);

            std::cout << "Connection closed" << std::endl;
        }
    }

    CloseHandle( pipe );

    // Close process and thread handles. 
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
}
