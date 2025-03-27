#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <conio.h>
#include <atomic>

DWORD originalAttr = 0;

DWORD colorCodeToAttribute(unsigned char code)
{
	if (code == '0')
		return originalAttr;
	
	if (code == '1')
		return BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;

	if(code == '2')
		return 0;

	if (code == '3')
		return BACKGROUND_BLUE | BACKGROUND_GREEN;

	if (code == '4')
		return BACKGROUND_BLUE | BACKGROUND_RED | BACKGROUND_INTENSITY;

	if (code == '5')
		return BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY;

	if (code == '6')
		return BACKGROUND_RED | BACKGROUND_INTENSITY;

	return BACKGROUND_RED;
}


void saveConsoleColor()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
	if (originalAttr == 0)
	{
		originalAttr = csbi.wAttributes;
	}
}

void setBgColor(DWORD bgAttribute)
{
    COORD coordScreen = { 0, 0 };  
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    GetConsoleScreenBufferInfo(hConsole, &csbi);
    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
	
	csbi.wAttributes &= 0xFF0F;  // zero the background color
	csbi.wAttributes |= bgAttribute;

    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);
	SetConsoleTextAttribute(hConsole, csbi.wAttributes); 
}

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
	BOOL fSuccess = FALSE;
	DWORD cbRead, dwMode;
	LPTSTR pipename = TEXT("\\\\.\\pipe\\colorpipe");

	std::cout << "Waiting for connection..." << std::endl;
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
			std::cout << "Could not open pipe. GLE=" <<  GetLastError() << std::endl;
			return -1;
		}

		// All pipe instances are busy, so wait for 20 seconds.
		if (!WaitNamedPipe(pipename, 20000))
		{
			printf("Could not open pipe: 20 second wait timed out.");
			return -1;
		}
	}

	std::cout << "Connection established" << std::endl;

	// The pipe connected; change to message-read mode.
	dwMode = PIPE_READMODE_MESSAGE;
	fSuccess = SetNamedPipeHandleState(
		pipe,	 // pipe handle
		&dwMode, // new pipe mode
		NULL,	 // don't set maximum bytes
		NULL);	 // don't set maximum time

	if (!fSuccess)
	{
		std::cout << "SetNamedPipeHandleState failed. GLE=" << GetLastError() << std::endl;
		return -1;
	}

	while (!isInterrupted)
	{
		// Read from the pipe.
		unsigned char chBuf[1];

		fSuccess = PeekNamedPipe(pipe, chBuf, sizeof(unsigned char), &cbRead, NULL, NULL);

		if (!fSuccess)
		{
			HRESULT err = GetLastError();
			if (err == ERROR_PIPE_NOT_CONNECTED)
				std::cout << "Server disconnected." << std::endl;
			else
				std::cout << "ReadFile from pipe failed. GLE=" <<  GetLastError() << std::endl;

			break;
		}

		if (cbRead > 0)
		{
			fSuccess = ReadFile(
				pipe,					 // pipe handle
				chBuf,					 // buffer to receive reply
				sizeof(unsigned char), // size of buffer
				&cbRead,				 // number of bytes read
				NULL);					 // not overlapped

			std::cout << "Accepted message: " << chBuf[0] << std::endl;
	
			saveConsoleColor();
	
			setBgColor(colorCodeToAttribute(chBuf[0]));
		}
	}

	std::cout << std::endl << "<Press ENTER to terminate connection and exit>" << std::endl;
	_getch();

	CloseHandle(pipe);
}
