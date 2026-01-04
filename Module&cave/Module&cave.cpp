#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <tchar.h>
#include <stdio.h>

void printError(TCHAR const* msg)
{
	DWORD eNum;
	TCHAR sysMsg[256];
	TCHAR* p;
	eNum = GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, eNum, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), sysMsg, 256, NULL);
	// trim the end of the line and terminate it with a null
	p = sysMsg;
	while ((*p > 31) || (*p == 9)) ++p;
	do { *p-- = 0; } while ((p >= sysMsg) && ((*p == '.') || (*p < 33)));

	// display the message
	_tprintf(TEXT("\n WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg);
}

BOOL ListProcessModules(DWORD pid)
{
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32;

	// take a snapshot at all modules in the specified process, see https://learn.microsoft.com/en-us/windows/win32/toolhelp/traversing-the-module-list
	hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if (hModuleSnap == INVALID_HANDLE_VALUE)
	{
		printError(TEXT("CreateToolhelp32Snapshot (of modules)"));
		return(FALSE);
	}
	me32.dwSize = sizeof(MODULEENTRY32);
	// retrieve information about the first module, and exit if unsucessfull
	if (!Module32First(hModuleSnap, &me32))
	{
		printError(TEXT("Module32First")); // show cause of failure
		CloseHandle(hModuleSnap); // must clean up the snapshot object
		return(FALSE);
	}
			// now walk the module list of the process, and display information about each module
	do
		{
		 _tprintf(TEXT("\n\n     MODULE NAME:     %s"), me32.szModule);
		 _tprintf(TEXT("\n     executable     = %s"), me32.szExePath);		
		 _tprintf(TEXT("\n     process ID     = 0x%08X"), me32.th32ProcessID);
		 _tprintf(TEXT("\n     ref count (g)  =     0x%04X"), me32.GlblcntUsage);
		 _tprintf(TEXT("\n     ref count (p)  =     0x%04X"), me32.ProccntUsage);
		 _tprintf(TEXT("\n     base address   = 0x%08X"), (DWORD)me32.modBaseAddr);
		 _tprintf(TEXT("\n     base size      = %d"), me32.modBaseSize);
		} while (Module32Next(hModuleSnap, &me32));
		 _tprintf(TEXT("\n"));

		 CloseHandle(hModuleSnap);
		 return(TRUE);
	
}

BOOL isthisGoodPid(DWORD pid) {
	char inp;
	std::cout << "is " << pid << " the good one?\n";
	std::cin >> inp;
	if (inp == 'y') {
		return true;
	}
	else {
		return false;
	}
}

void run_hidden_nowait() {
	STARTUPINFOW si{};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

	PROCESS_INFORMATION pi{};
	DWORD flags = CREATE_NO_WINDOW;

	wchar_t cmd[] = L"cmd.exe /c Taskmgr.exe"; // can use another process, but i need this process to do my project
	if (CreateProcessW(nullptr, cmd, nullptr, nullptr, FALSE, flags, nullptr, nullptr, &si, &pi)) {
		// do not wait, close handles to avoid leaks. FOUND on daniweb.com -> https://www.daniweb.com/programming/software-development/threads/81409/execute-command-or-application-without-waiting
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}

DWORD GetProcessIdByName(const std::string& processName)
{
	DWORD processId = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (snapshot == INVALID_HANDLE_VALUE) {
		return 0;
	}

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(snapshot, &pe))
	{
		do {
			char exeName[MAX_PATH];
			WideCharToMultiByte(CP_ACP, 0, pe.szExeFile, -1, exeName, MAX_PATH, nullptr, nullptr);

			if (_strcmpi(exeName, processName.c_str()) == 0)
			{
				processId = pe.th32ProcessID;
				break;
			}
		} while (Process32Next(snapshot, &pe));
	}

	CloseHandle(snapshot);
	return processId;
}

bool endsWithExe(const std::string& str) {
	if (str.length() < 4) {
		return false;
	}
	std::string ending = str.substr(str.length() - 4);
	for (size_t i = 0; i < ending.length(); i++) {
		ending[i] = tolower(static_cast<unsigned char>(ending[i]));
	}
	return ending == ".exe";
}
	// its working so lets get the process id
	// get processbyname requires two parameters, both are string, i dont know why it requires a machineName variable but anyways, actually um machineName could be null 0
	// i think the getprocessbyname is only available on C# ( at this moment i regret not using C# lmao )

int main() {
	std::cout << "I will start taskmanager because i need u to verify something\n";
	// use the non blocking function so i CAN CONTINUE my own program instead of system()
	run_hidden_nowait();
	// give it a moment to start
	Sleep(1000);
	/*
	std::cout << "type the process name: \n";
	std::cin >> processName;
	std::cout << "the process name is " << processName << "\n"; // this is for debug purposes, do not remove it
	*/
	std::string processName; // avoid using char and thats why i used widechar.. func because it has dynamic allocation, bounds checking, char does not, we cant know what the user will type so .. If you dont care then hackers could use Code Caves then overvride, see bufferoverflow vulnerability at https://www.fortinet.com/resources/cyberglossary/buffer-overflow

	bool validInput = false;
	while (!validInput) {
		std::cout << "type the process name ( must end with .exe): ";
		std::cin >> processName;

		if (endsWithExe(processName)) {
			validInput = true;
			std::cout << "The process name is " << processName << "\n"; 
		}
		else {
			std::cout << "DEBUG: Process name must end with .exe extension ( else the process is not found )\n";
			std::cout << "DEBUG: Taskmgr.exe, chrome.exe, notepad.exe\n\n";
		}
	}
	DWORD pid = GetProcessIdByName(processName);
	if (isthisGoodPid(pid)) {
		std::cout << "fine";
	}
	else {
		std::cout << "let me know the pid of ur target software\n";
		std::cin >> pid;
	}
	ListProcessModules(pid);
	return 0;
}