/*
Run "python3 BasicAuthHTTPServer.py 80" on the server defined by the macro HOST
Run Wireshark with the capture filter "host x.x.x.x" where x.x.x.x is the server defined by the macro HOST
Compile & Run
*/

#include <iostream>

#undef UNICODE
#include <Windows.h>
#include <wininet.h>

#define HOST "127.0.0.1"
#define PATH "/"
#define USERNAME "demo"
#define PASSWORD "demo"


void GetRestrictedResource()
{
    HINTERNET hOpenHandle, hResourceHandle, hConnectHandle;
    DWORD dwStatus;
    DWORD dwStatusSize = sizeof(DWORD);
    char strUsername[64], strPassword[64];

    hOpenHandle = InternetOpen(TEXT("Example"),
        INTERNET_OPEN_TYPE_PRECONFIG,
        NULL, NULL, 0);
    hConnectHandle = InternetConnect(hOpenHandle,
        TEXT(HOST),
        INTERNET_INVALID_PORT_NUMBER,
        NULL,
        NULL,
        INTERNET_SERVICE_HTTP,
        0, 0);

    hResourceHandle = HttpOpenRequest(hConnectHandle, TEXT("GET"),
        TEXT(PATH),
        NULL, NULL, NULL,
        INTERNET_FLAG_KEEP_CONNECTION,
        0);

resend:

    HttpSendRequest(hResourceHandle, NULL, 0, NULL, 0);

    HttpQueryInfo(hResourceHandle, HTTP_QUERY_FLAG_NUMBER |
        HTTP_QUERY_STATUS_CODE, &dwStatus, &dwStatusSize, NULL);

    switch (dwStatus)
    {
        DWORD cchUserLength, cchPasswordLength;

    case HTTP_STATUS_DENIED:     // Server Authentication Required.
        strcpy_s(strUsername, USERNAME);
        cchUserLength = strlen(strUsername);
        strcpy_s(strPassword, PASSWORD);
        cchPasswordLength = strlen(strPassword);

        InternetSetOption(hResourceHandle, INTERNET_OPTION_USERNAME,
            strUsername, cchUserLength + 1);
        InternetSetOption(hResourceHandle, INTERNET_OPTION_PASSWORD,
            strPassword, cchPasswordLength + 1);
        goto resend;
        break;
    }
}

int main()
{
    // Simulates IE accessing a restricted HTTP resource
    // 1. Access restricted HTTP resource (without authentication) -> 401 Unauthorized
    // 2. retry with authentication -> 200 OK
    GetRestrictedResource();

    Sleep(5*1000);

    // Simulates an attacker injecting code to access the restricted HTTP resource
    // 1. Access restricted HTTP resource (with saved authentication thanks to WinINet) in a new thread -> 200 OK
    if (HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)GetRestrictedResource, NULL, 0, NULL))
    {
        WaitForSingleObject(hThread, INFINITE);
    }
}
