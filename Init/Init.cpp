#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <iostream>
#include <thread>

// Function to terminate a specific process by name
void TerminateProcessByName(const std::wstring& processName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Failed to create snapshot" << std::endl;
        return;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnap, &pe)) {
        do {
            if (std::wstring(pe.szExeFile) == processName) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProcess != NULL) {
                    if (TerminateProcess(hProcess, 0)) {
                        std::wcout << L"Terminated process: " << processName << std::endl;
                    }
                    else {
                        std::wcerr << L"Failed to terminate process: " << processName << std::endl;
                    }
                    CloseHandle(hProcess);
                }
                else {
                    std::wcerr << L"Failed to open process: " << processName << std::endl;
                }
            }
        } while (Process32Next(hSnap, &pe));
    }
    else {
        std::wcerr << L"Failed to retrieve first process" << std::endl;
    }

    CloseHandle(hSnap);
}

// Function to check if Task Manager is running
bool IsTaskManagerRunning() {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    bool isRunning = false;

    if (Process32First(hSnap, &pe)) {
        do {
            if (std::wstring(pe.szExeFile) == L"Taskmgr.exe") {
                isRunning = true;
                break;
            }
        } while (Process32Next(hSnap, &pe));
    }

    CloseHandle(hSnap);
    return isRunning;
}

// Function to restart Task Manager if it is not running
void RestartTaskManagerIfNeeded() {
    if (!IsTaskManagerRunning()) {
        std::wcout << L"Task Manager is not running. Restarting Task Manager." << std::endl;
        if (!ShellExecuteW(NULL, L"open", L"taskmgr.exe", NULL, NULL, SW_SHOWNORMAL)) {
            std::wcerr << L"Failed to restart Task Manager" << std::endl;
        }
    }
}

// Function to monitor and terminate specific processes, and ensure Task Manager is running
void MonitorAndTerminateProcesses(const std::vector<std::wstring>& processNames) {
    while (true) {
        for (const auto& processName : processNames) {
            TerminateProcessByName(processName);
        }
        RestartTaskManagerIfNeeded();
        Sleep(1000); // Polling every second
    }
}

// Registry value setting function
void SetRegistryValue(HKEY root, LPCWSTR subKey, LPCWSTR valueName, DWORD data) {
    HKEY hKey;
    if (RegOpenKeyEx(root, subKey, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, valueName, 0, REG_DWORD, (const BYTE*)&data, sizeof(data));
        RegCloseKey(hKey);
    }
    else {
        wprintf(L"Unable to open registry key: %s\n", subKey);
    }
}

// Function to set mouse acceleration
void SetMouseAcceleration(BOOL mouseAccel) {
    int mouseParams[3];
    SystemParametersInfo(SPI_GETMOUSE, 0, mouseParams, 0);
    mouseParams[2] = mouseAccel;
    SystemParametersInfo(SPI_SETMOUSE, 0, mouseParams, SPIF_SENDCHANGE);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Disable enhanced pointer precision
    SetMouseAcceleration(false);

    // Set dark mode for apps
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", 0);

    // Set Taskbar buttons to always combine
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"TaskbarGlomLevel", 0);

    // Processes to terminate
    std::vector<std::wstring> processesToTerminate = {
        L"lwclient64.exe",
        L"lwfunctionbar64.exe",
        L"lwhardware64.exe",
        L"lwBarClientApp32.exe"
    };

    // Start monitoring and terminating specified processes, and restarting Task Manager if needed
    std::thread monitorThread(MonitorAndTerminateProcesses, processesToTerminate);
    monitorThread.detach();

    // Keep main thread active
    while (true) {
        Sleep(1000);
    }

    return 0;
}
