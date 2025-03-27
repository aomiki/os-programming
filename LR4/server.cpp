#include <windows.h>
#include <iostream>
#include <atomic>

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
        TEXT("\\\\.\\pipe\\colorpipe"), PIPE_ACCESS_DUPLEX,
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

    std::cout << "Creating client..." << std::endl;
    // Start the child process. 
    if( !CreateProcess( NULL,   // No module name (use command line)
        TEXT("client.exe"),        // Command line
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

    if (pipe != INVALID_HANDLE_VALUE)
    {
        std::cout << "Waiting for connection" << std::endl;
        if (ConnectNamedPipe(pipe, NULL) != FALSE)   // wait for someone to connect to the pipe
        {
            while (!isInterrupted)
            {
                DWORD dwRead;

                std::cout << "Choose one color:" << std::endl;
                std::cout << "0. Restore original" << std::endl;
                std::cout << "1. White" << std::endl;
                std::cout << "2. Black" << std::endl;
                std::cout << "3. Bright blue" << std::endl;
                std::cout << "4. Magenta" << std::endl;
                std::cout << "5. Bright Yellow" << std::endl;
                std::cout << "6. Red" << std::endl;

                unsigned char color[1];
                std::cin >> color[0];

                if (isInterrupted) continue;

                if (color[0] < '0' || color[0] > '6')
                {
                    std::cout << "Number out of range" << std::endl;
                    continue;
                }

                if (!WriteFile(pipe, color, sizeof(unsigned char), &dwRead, NULL))
                {
                    std::cout << "Send error" << std::endl;
                }
            }
        }
    }

    DisconnectNamedPipe(pipe);

    std::cout << "Connection closed" << std::endl;

    CloseHandle( pipe );

    // Close process and thread handles. 
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
}
