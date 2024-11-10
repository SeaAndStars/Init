#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <iostream>
#include <thread>

// 终止指定名称的进程
void TerminateProcessByName(const std::wstring& processName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        std::wcerr << L"创建快照失败" << std::endl;
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
                        std::wcout << L"已终止进程: " << processName << std::endl;
                    }
                    else {
                        std::wcerr << L"终止进程失败: " << processName << std::endl;
                    }
                    CloseHandle(hProcess);
                }
                else {
                    std::wcerr << L"打开进程失败: " << processName << std::endl;
                }
            }
        } while (Process32Next(hSnap, &pe));
    }
    CloseHandle(hSnap);
}

// 设置注册表值的函数
void SetRegistryValue(HKEY root, LPCWSTR subKey, LPCWSTR valueName, DWORD data) {
    HKEY hKey;
    if (RegOpenKeyEx(root, subKey, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, valueName, 0, REG_DWORD, (const BYTE*)&data, sizeof(data));
        RegCloseKey(hKey);
    }
    else {
        wprintf(L"无法打开注册表项: %s\n", subKey);
    }
}

// 设置鼠标加速的函数
void SetMouseAcceleration(BOOL mouseAccel) {
    int mouseParams[3];
    SystemParametersInfo(SPI_GETMOUSE, 0, mouseParams, 0);
    mouseParams[2] = mouseAccel;
    SystemParametersInfo(SPI_SETMOUSE, 0, mouseParams, SPIF_SENDCHANGE);
}

// 重启 Explorer 进程的函数
void RestartExplorer() {
    // 先终止 explorer 进程
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "无法创建进程快照" << std::endl;
        return;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnap, &pe)) {
        do {
            if (std::wstring(pe.szExeFile) == L"explorer.exe") {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProcess != NULL) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                    std::cout << "已终止 Explorer 进程" << std::endl;
                }
            }
        } while (Process32Next(hSnap, &pe));
    }
    else {
        std::cerr << "无法检索第一个进程" << std::endl;
    }

    CloseHandle(hSnap);

    // 重新启动 explorer
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcess(L"C:\\Windows\\explorer.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        std::cout << "已重启 Explorer" << std::endl;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else {
        std::cerr << "重启 Explorer 失败" << std::endl;
    }
}

// 监控并终止特定进程
void MonitorAndTerminateProcesses(const std::vector<std::wstring>& processNames) {
    while (true) {
        for (const auto& processName : processNames) {
            TerminateProcessByName(processName);
        }
        Sleep(1000); // 每秒轮询一次
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 禁用增强指针精度（关闭鼠标加速）
    SetMouseAcceleration(false);

    // 设置应用程序深色模式
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", 0);

    // 设置任务栏按钮总是组合
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"TaskbarGlomLevel", 0);

    // 更改壁纸路径，示例（根据需要调整路径）
    const wchar_t* wallpaperPath = L"C:\\Windows\\Web\\Wallpaper\\Theme1\\img4.jpg";
    if (SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (PVOID)wallpaperPath, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
        wprintf(L"壁纸已更改为: %s\n", wallpaperPath);
    }
    else {
        std::wcerr << L"更改壁纸失败" << std::endl;
    }

    // 重启 Explorer
    RestartExplorer();

    // 需要终止的进程列表
    std::vector<std::wstring> processesToTerminate = {
        L"lwclient64.exe",
        L"lwfunctionbar64.exe",
        L"lwhardware64.exe",
        L"lwBarClientApp32.exe"
    };

    // 启动监控并终止指定的进程
    std::thread monitorThread(MonitorAndTerminateProcesses, processesToTerminate);
    monitorThread.detach(); // 分离线程，允许监控独立运行

    // 保持主线程活跃
    while (true) {
        Sleep(1000);
    }

    return 0;
}
