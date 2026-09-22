// file: editsettings.cpp
// Edit settings window procedure for showing controls / settings

#include <windows.h>
#include <string>
#include <vector>
#include <winver.h>
#include <commdlg.h>

#pragma comment(lib, "Version.lib")
#pragma comment(lib, "comdlg32.lib")

//fetch the variables from the config file
void GetTextColorFromConfig(char* buffer, size_t size);
void GetBackgroundColorFromConfig(char* buffer, size_t size);

static std::wstring GetProgramVersion()
{
    wchar_t modulePath[MAX_PATH] = {};
    if (GetModuleFileNameW(nullptr, modulePath, _countof(modulePath)) == 0)
        return L"Unknown";

    DWORD dummy = 0;
    DWORD versionInfoSize = GetFileVersionInfoSizeW(modulePath, &dummy);
    if (versionInfoSize == 0)
        return L"Unknown";

    std::vector<BYTE> versionInfo(versionInfoSize);
    if (!GetFileVersionInfoW(modulePath, 0, versionInfoSize, versionInfo.data()))
        return L"Unknown";

    VS_FIXEDFILEINFO* fileInfo = nullptr;
    UINT fileInfoSize = 0;
    if (!VerQueryValueW(
        versionInfo.data(),
        L"\\",
        reinterpret_cast<LPVOID*>(&fileInfo),
        &fileInfoSize) ||
        fileInfo == nullptr)
    {
        return L"Unknown";
    }

    return std::to_wstring(HIWORD(fileInfo->dwFileVersionMS)) + L"." +
        std::to_wstring(LOWORD(fileInfo->dwFileVersionMS)) + L"." +
        std::to_wstring(HIWORD(fileInfo->dwFileVersionLS)) + L"." +
        std::to_wstring(LOWORD(fileInfo->dwFileVersionLS));
}

constexpr int ID_CHANGE_SETTINGS = 1001;
constexpr int ID_SAVE = 1002;
constexpr int ID_CANCEL = 1003;
constexpr int ID_TEXT_COLOR_EDIT = 1004;
constexpr int ID_BG_COLOR_EDIT = 1005;
constexpr int ID_TEXT_COLOR_PICKER = 1006;
constexpr int ID_BG_COLOR_PICKER = 1007;

HWND g_changeSettingsWindow = nullptr;

// Helper function to convert COLORREF to hex string
void ColorToHexString(COLORREF color, char* buffer, size_t size)
{
    snprintf(buffer, size, "#%02X%02X%02X",
        GetRValue(color), GetGValue(color), GetBValue(color));
}

// Helper function to parse hex string to COLORREF
COLORREF HexStringToColor(const char* hex)
{
    if (!hex || *hex != '#' || strlen(hex) != 7)
        return RGB(0, 0, 0);

    unsigned int r = 0, g = 0, b = 0;
    if (sscanf_s(hex + 1, "%02X%02X%02X", &r, &g, &b) == 3)
    {
        return RGB(
            static_cast<BYTE>(r),
            static_cast<BYTE>(g),
            static_cast<BYTE>(b)
        );
    }
    return RGB(0, 0, 0);
}

LRESULT CALLBACK ChangeSettingsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        // Create Text Color label and edit control
        CreateWindowW(
            L"STATIC",
            L"Text Color:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, 20, 120, 20,
            hWnd,
            reinterpret_cast<HMENU>(0),
            GetModuleHandleW(nullptr),
            nullptr);

        CreateWindowW(
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            150, 20, 100, 20,
            hWnd,
            reinterpret_cast<HMENU>(ID_TEXT_COLOR_EDIT),
            GetModuleHandleW(nullptr),
            nullptr);

        // Create Text Color picker button
        CreateWindowW(
            L"BUTTON",
            L"...",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            255, 20, 30, 20,
            hWnd,
            reinterpret_cast<HMENU>(ID_TEXT_COLOR_PICKER),
            GetModuleHandleW(nullptr),
            nullptr);

        // Create Background Color label and edit control
        CreateWindowW(
            L"STATIC",
            L"Background Color:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, 50, 120, 20,
            hWnd,
            reinterpret_cast<HMENU>(0),
            GetModuleHandleW(nullptr),
            nullptr);

        CreateWindowW(
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            150, 50, 100, 20,
            hWnd,
            reinterpret_cast<HMENU>(ID_BG_COLOR_EDIT),
            GetModuleHandleW(nullptr),
            nullptr);

        // Create Background Color picker button
        CreateWindowW(
            L"BUTTON",
            L"...",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            255, 50, 30, 20,
            hWnd,
            reinterpret_cast<HMENU>(ID_BG_COLOR_PICKER),
            GetModuleHandleW(nullptr),
            nullptr);

        // Create Save button
        CreateWindowW(
            L"BUTTON",
            L"Save",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            50, 100, 60, 25,
            hWnd,
            reinterpret_cast<HMENU>(ID_SAVE),
            GetModuleHandleW(nullptr),
            nullptr);

        // Create Cancel button
        CreateWindowW(
            L"BUTTON",
            L"Cancel",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            150, 100, 60, 25,
            hWnd,
            reinterpret_cast<HMENU>(ID_CANCEL),
            GetModuleHandleW(nullptr),
            nullptr);

        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_TEXT_COLOR_PICKER:
        {
            CHOOSECOLOR cc = {};
            COLORREF customColors[16] = {
            RGB(255, 0, 0), RGB(0, 255, 0), RGB(0, 0, 255), RGB(255, 255, 0),
            RGB(255, 0, 255), RGB(0, 255, 255), RGB(255, 255, 255), RGB(192, 192, 192),
            RGB(128, 128, 128), RGB(0, 0, 0), RGB(255, 192, 192), RGB(192, 255, 192),
            RGB(192, 192, 255), RGB(255, 255, 192), RGB(255, 192, 255), RGB(192, 255, 255)
            };

            COLORREF initialColor = RGB(0, 0, 0);

            // Try to parse current text color
            char currentColor[256] = {};
            GetWindowTextA(GetDlgItem(hWnd, ID_TEXT_COLOR_EDIT), currentColor, sizeof(currentColor));
            if (currentColor[0])
                initialColor = HexStringToColor(currentColor);

            cc.lStructSize = sizeof(CHOOSECOLOR);
            cc.hwndOwner = hWnd;
            cc.lpCustColors = customColors;
            cc.rgbResult = initialColor;
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;

            if (ChooseColor(&cc))
            {
                char colorRgb[32];
                sprintf_s(
                    colorRgb,
                    "%u,%u,%u",
                    static_cast<unsigned int>(GetRValue(cc.rgbResult)),
                    static_cast<unsigned int>(GetGValue(cc.rgbResult)),
                    static_cast<unsigned int>(GetBValue(cc.rgbResult)));

                SetWindowTextA(
                    GetDlgItem(hWnd, ID_TEXT_COLOR_EDIT), // Use ID_BG_COLOR_EDIT in the background picker
                    colorRgb);
            }
            return 0;
        }

        case ID_BG_COLOR_PICKER:
        {
            CHOOSECOLOR cc = {};
            COLORREF customColors[16] = {
            RGB(255, 0, 0), RGB(0, 255, 0), RGB(0, 0, 255), RGB(255, 255, 0),
            RGB(255, 0, 255), RGB(0, 255, 255), RGB(255, 255, 255), RGB(192, 192, 192),
            RGB(128, 128, 128), RGB(0, 0, 0), RGB(255, 192, 192), RGB(192, 255, 192),
            RGB(192, 192, 255), RGB(255, 255, 192), RGB(255, 192, 255), RGB(192, 255, 255)
            };
            COLORREF initialColor = RGB(255, 255, 255);

            // Try to parse current background color
            char currentColor[256] = {};
            GetWindowTextA(GetDlgItem(hWnd, ID_BG_COLOR_EDIT), currentColor, sizeof(currentColor));
            if (currentColor[0])
                initialColor = HexStringToColor(currentColor);

            cc.lStructSize = sizeof(CHOOSECOLOR);
            cc.hwndOwner = hWnd;
            cc.lpCustColors = customColors;
            cc.rgbResult = initialColor;
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;

            if (ChooseColor(&cc))
            {
                char colorRgb[32];
                sprintf_s(
                    colorRgb,
                    "%u,%u,%u",
                    static_cast<unsigned int>(GetRValue(cc.rgbResult)),
                    static_cast<unsigned int>(GetGValue(cc.rgbResult)),
                    static_cast<unsigned int>(GetBValue(cc.rgbResult)));

                SetWindowTextA(
                    GetDlgItem(hWnd, ID_BG_COLOR_EDIT),
                    colorRgb);
            }
            return 0;
        }

        case ID_SAVE:
        {
            // Read text color from edit control
            char textColor[256] = {};
            GetWindowTextA(GetDlgItem(hWnd, ID_TEXT_COLOR_EDIT), textColor, sizeof(textColor));

            // Read background color from edit control
            char bgColor[256] = {};
            GetWindowTextA(GetDlgItem(hWnd, ID_BG_COLOR_EDIT), bgColor, sizeof(bgColor));

            // Save to config file
            const char* name = "config.txt";
            char path[MAX_PATH] = { 0 };
            char mod[MAX_PATH] = { 0 };
            if (GetModuleFileNameA(NULL, mod, MAX_PATH) != 0)
            {
                char* p = strrchr(mod, '\\');
                if (p)
                {
                    *++p = '\0';
                    strcpy_s(path, sizeof(path), mod);
                    strcat_s(path, sizeof(path), name);
                }
                else
                {
                    strcpy_s(path, sizeof(path), name);
                }
            }
            else
            {
                GetCurrentDirectoryA(MAX_PATH, path);
                size_t len = strlen(path);
                if (len && path[len - 1] != '\\') strcat_s(path, sizeof(path), "\\");
                strcat_s(path, sizeof(path), name);
            }

            HANDLE hFile = CreateFileA(
                path,
                GENERIC_WRITE,
                0,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

            if (hFile != INVALID_HANDLE_VALUE)
            {
                SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

                // Write text_color line
                char writeBuffer[256];
                sprintf_s(writeBuffer, "text_color=%s\n", textColor);
                DWORD written = 0;
                WriteFile(hFile, writeBuffer, (DWORD)strlen(writeBuffer), &written, NULL);

                // Write bg_color line
                sprintf_s(writeBuffer, "bg_color=%s\n", bgColor);
                WriteFile(hFile, writeBuffer, (DWORD)strlen(writeBuffer), &written, NULL);

                CloseHandle(hFile);
            }

            DestroyWindow(hWnd);
            return 0;
        }
        case ID_CANCEL:
            DestroyWindow(hWnd);
            return 0;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        g_changeSettingsWindow = nullptr;
        return 0;
    }

    return DefWindowProcW(hWnd, message, wParam, lParam);
}

void OpenChangeSettingsWindow(HWND owner)
{
    if (g_changeSettingsWindow != nullptr)
    {
        SetForegroundWindow(g_changeSettingsWindow);
        return;
    }

    static bool classRegistered = false;

    if (!classRegistered)
    {
        WNDCLASSW windowClass = {};
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.lpfnWndProc = ChangeSettingsWndProc;
        windowClass.lpszClassName = L"MiniMonitorChangeSettingsWindow";
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

        RegisterClassW(&windowClass);
        classRegistered = true;
    }

    POINT cursorPosition;
    GetCursorPos(&cursorPosition);

    // Get screen dimensions to ensure window stays within bounds
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Window size constants
    const int windowWidth = 300;
    const int windowHeight = 200;

    // Calculate position to keep window fully within screen
    int x = cursorPosition.x;
    int y = cursorPosition.y;

    // Adjust if window would extend beyond right edge
    if (x + windowWidth > screenWidth)
    {
        x = screenWidth - windowWidth - 10; // Leave some margin from right edge
    }

    // Adjust if window would extend beyond bottom edge
    if (y + windowHeight > screenHeight)
    {
        y = screenHeight - windowHeight - 10; // Leave some margin from bottom edge
    }

    g_changeSettingsWindow = CreateWindowExW(
        0,
        L"MiniMonitorChangeSettingsWindow",
        L"Change settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        x,
        y,
        windowWidth,
        windowHeight,
        owner,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr);

    if (g_changeSettingsWindow != nullptr)
    {
        ShowWindow(g_changeSettingsWindow, SW_SHOW);
        UpdateWindow(g_changeSettingsWindow);

        // Load current settings from config file into edit controls
        char textColorBuffer[256] = {};
        GetTextColorFromConfig(textColorBuffer, sizeof(textColorBuffer));
        SetWindowTextA(GetDlgItem(g_changeSettingsWindow, ID_TEXT_COLOR_EDIT), textColorBuffer);

        char bgColorBuffer[256] = {};
        GetBackgroundColorFromConfig(bgColorBuffer, sizeof(bgColorBuffer));
        SetWindowTextA(GetDlgItem(g_changeSettingsWindow, ID_BG_COLOR_EDIT), bgColorBuffer);
    }
}