#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>

// Function to set the registry value
void SetRegistryValue(HKEY root, LPCWSTR subKey, LPCWSTR valueName, DWORD data) {
    HKEY hKey;
    if (RegOpenKeyEx(root, subKey, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, valueName, 0, REG_DWORD, (const BYTE*)&data, sizeof(data));
        RegCloseKey(hKey);
    }
    else {
        wprintf(L"Failed to open registry key: %s\n", subKey);
    }
}

// Function to change wallpaper
void SetWallpaper(LPCWSTR wallpaperPath) {
    if (SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (PVOID)wallpaperPath, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
        wprintf(L"Wallpaper changed to: %s\n", wallpaperPath);
    }
    else {
        wprintf(L"Failed to change wallpaper to: %s\n", wallpaperPath);
    }
}

// Mutex for synchronizing wallpaper changes
std::mutex wallpaperMutex;

// Atomic boolean to stop threads
std::atomic<bool> stopThreads(false);

// Thread function to change wallpaper as fast as possible
void WallpaperChanger(const wchar_t* wallpaperPath) {
    while (!stopThreads.load()) {
        {
            std::lock_guard<std::mutex> lock(wallpaperMutex);
            SetWallpaper(wallpaperPath);
        }
        // Busy-wait loop for high frequency (not recommended for long-term use)
    }
}
// Turns mouse acceleration on/off by calling the SystemParametersInfo function.
// When mouseAccel is TRUE, mouse acceleration is turned on; FALSE for off.
void SetMouseAcceleration(BOOL mouseAccel)
{
    int mouseParams[3];

    // Get the current values.
    SystemParametersInfo(SPI_GETMOUSE, 0, mouseParams, 0);

    // Modify the acceleration value as directed.
    mouseParams[2] = mouseAccel;

    // Update the system setting.
    SystemParametersInfo(SPI_SETMOUSE, 0, mouseParams, SPIF_SENDCHANGE);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    //// Disable enhance pointer precision
    //SetRegistryValue(HKEY_CURRENT_USER, L"Control Panel\\Mouse", L"MouseSpeed", 1); // Typically, MouseSpeed should be set to 1
    //SetRegistryValue(HKEY_CURRENT_USER, L"Control Panel\\Mouse", L"MouseThreshold1", 6); // Typically, MouseThreshold1 should be set to 6
    ////SetRegistryValue(HKEY_CURRENT_USER, L"Control Panel\\Mouse", L"MouseThreshold2", 10); // Typically, MouseThreshold2 should be set to 10
    //SetRegistryValue(HKEY_CURRENT_USER, L"Control Panel\\Mouse", L"MousePrecision", 0); // This is the key to disable Enhance Pointer Precision
    SetMouseAcceleration(false);

    // Set default app mode to dark
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", 0);

    // Set default browser to Edge
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\http\\UserChoice", L"ProgId", (DWORD)L"MSEdgeHTM");
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\https\\UserChoice", L"ProgId", (DWORD)L"MSEdgeHTM");
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\ftp\\UserChoice", L"ProgId", (DWORD)L"MSEdgeHTM");

    // Set taskbar buttons to always combine
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"TaskbarGlomLevel", 0);

    // Number of threads to create
    const int numThreads = 10;
    const wchar_t* wallpaperPath = L"C:\\Windows\\Web\\Wallpaper\\Theme1\\img4.jpg";
    std::vector<std::thread> threads;

    // Create and detach threads
    for (int i = 0; i < numThreads; ++i) {
        threads.push_back(std::thread(WallpaperChanger, wallpaperPath));
        threads.back().detach();
    }

    // Main thread can continue with other tasks or exit
    // To keep the main thread alive if needed:
    while (true) {
        Sleep(1000); // Sleep for 1 second
    }

    // Signal threads to stop (not reached in this example)
    stopThreads.store(true);
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    return 0;
}
