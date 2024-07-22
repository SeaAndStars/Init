#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

// 终止指定名称的进程的函数
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
    else {
        std::wcerr << L"检索第一个进程失败" << std::endl;
    }

    CloseHandle(hSnap);
}

// 持续监控并终止特定进程的函数
void MonitorAndTerminateProcesses(const std::vector<std::wstring>& processNames) {
    while (true) {
        for (const auto& processName : processNames) {
            TerminateProcessByName(processName);
        }
        Sleep(1000); // 每秒轮询一次
    }
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

// 更改壁纸的函数
void SetWallpaper(LPCWSTR wallpaperPath) {
    if (SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (PVOID)wallpaperPath, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
        wprintf(L"壁纸已更改为: %s\n", wallpaperPath);
    }
    else {
        wprintf(L"更改壁纸失败: %s\n", wallpaperPath);
    }
}

// 用于同步壁纸更改的互斥锁
std::mutex wallpaperMutex;

// 原子布尔值，用于停止线程
std::atomic<bool> stopThreads(false);

// 更改壁纸的线程函数，尽可能快地更改壁纸
void WallpaperChanger(const wchar_t* wallpaperPath) {
    while (!stopThreads.load()) {
        {
            std::lock_guard<std::mutex> lock(wallpaperMutex);
            SetWallpaper(wallpaperPath);
        }
        // 忙等待循环以高频率运行（不推荐长期使用）
    }
}

// 设置鼠标加速开关的函数
void SetMouseAcceleration(BOOL mouseAccel) {
    int mouseParams[3];

    // 获取当前值
    SystemParametersInfo(SPI_GETMOUSE, 0, mouseParams, 0);

    // 根据需要修改加速值
    mouseParams[2] = mouseAccel;

    // 更新系统设置
    SystemParametersInfo(SPI_SETMOUSE, 0, mouseParams, SPIF_SENDCHANGE);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 禁用增强指针精确度
    // SetRegistryValue(HKEY_CURRENT_USER, L"Control Panel\\Mouse", L"MouseSpeed", 1); // 通常，MouseSpeed 应设置为 1
    // SetRegistryValue(HKEY_CURRENT_USER, L"Control Panel\\Mouse", L"MouseThreshold1", 6); // 通常，MouseThreshold1 应设置为 6
    // SetRegistryValue(HKEY_CURRENT_USER, L"Control Panel\\Mouse", L"MouseThreshold2", 10); // 通常，MouseThreshold2 应设置为 10
    // SetRegistryValue(HKEY_CURRENT_USER, L"Control Panel\\Mouse", L"MousePrecision", 0); // 这是禁用增强指针精确度的关键
    SetMouseAcceleration(false);

    // 设置默认应用模式为深色模式
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", 0);

    // 将默认浏览器设置为 Edge
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\http\\UserChoice", L"ProgId", (DWORD)L"MSEdgeHTM");
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\https\\UserChoice", L"ProgId", (DWORD)L"MSEdgeHTM");
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\ftp\\UserChoice", L"ProgId", (DWORD)L"MSEdgeHTM");

    // 设置任务栏按钮始终合并
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"TaskbarGlomLevel", 0);

    // 要创建的线程数
    const int numThreads = 10;
    const wchar_t* wallpaperPath = L"C:\\Windows\\Web\\Wallpaper\\Theme1\\img4.jpg";
    std::vector<std::thread> threads;

    // 创建并分离线程
    for (int i = 0; i < numThreads; ++i) {
        threads.push_back(std::thread(WallpaperChanger, wallpaperPath));
        threads.back().detach();
    }

    // 主线程可以继续执行其他任务或退出
    // 为了让主线程保持活跃：
    std::vector<std::wstring> processesToTerminate = {
        L"lwclient64.exe",
        L"lwfunctionbar64.exe",
        L"lwhardware64.exe"
    };

    // 启动监控和终止线程
    std::thread monitorThread(MonitorAndTerminateProcesses, processesToTerminate);
    monitorThread.detach(); // 分离监控线程

    // 保持主线程活跃
    while (true) {
        Sleep(1000); // 每秒休眠 1 秒
    }

    // 向线程发送停止信号（在本例中不会到达）
    stopThreads.store(true);
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    return 0;
}
