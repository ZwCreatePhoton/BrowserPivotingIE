#include <iostream>

#include <Windows.h>

#define DLLPATH "HTTPProxy.dll"

// Test Program to host the HTTP proxy.

int main()
{
    LoadLibraryA(DLLPATH);
    Sleep(60*60*1000);
}
