#include <windows.h>
#include <stdio.h>
#include <tchar.h>

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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Close precision mouse
    SetRegistryValue(HKEY_CURRENT_USER, L"Control Panel\\Mouse", L"MouseSpeed", 0);
    SetRegistryValue(HKEY_CURRENT_USER, L"Control Panel\\Mouse", L"MouseThreshold1", 0);
    SetRegistryValue(HKEY_CURRENT_USER, L"Control Panel\\Mouse", L"MouseThreshold2", 0);

    // Set default app mode to dark
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", 0);

    // Set default browser to Edge
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\http\\UserChoice", L"ProgId", (DWORD)L"MSEdgeHTM");
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\https\\UserChoice", L"ProgId", (DWORD)L"MSEdgeHTM");
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\ftp\\UserChoice", L"ProgId", (DWORD)L"MSEdgeHTM");

    // Set taskbar buttons to always combine
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"TaskbarGlomLevel", 0);

    // Change wallpaper periodically
    const wchar_t* wallpaperPath = L"C:\\Windows\\Web\\Wallpaper\\Theme1\\img4.jpg";
    while (1) {
        SetWallpaper(wallpaperPath);
        Sleep(1); // Sleep for 10 seconds
    }

    return 0;
}
