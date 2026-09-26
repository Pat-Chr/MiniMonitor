// file: editsettings.cpp
// Edit settings window procedure for showing controls / settings

#include <windows.h>
#include <string>
#include <vector>
#include <winver.h>
#include <commdlg.h>
#include <fstream>

#pragma comment(lib, "Version.lib")
#pragma comment(lib, "comdlg32.lib")

#include "saveasetting.h";
// Global function to save a setting to the configuration file. Can be used for any setting.
void SaveASetting(const std::wstring& key, const std::wstring& value);
// Example usage: SaveASetting(L"settingName", L"settingValue");

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
            char textColor[256] = {};
            GetWindowTextA(
                GetDlgItem(hWnd, ID_TEXT_COLOR_EDIT),
                textColor,
                sizeof(textColor));

            char bgColor[256] = {};
            GetWindowTextA(
                GetDlgItem(hWnd, ID_BG_COLOR_EDIT),
                bgColor,
                sizeof(bgColor));

            std::ifstream inputFile("config.txt");
            std::vector<std::string> lines;
            std::string line;
            bool foundTextColor = false;
            bool foundBgColor = false;

            while (std::getline(inputFile, line))
            {
                if (line.rfind("text_color=", 0) == 0)
                {
                    line = "text_color=" + std::string(textColor);
                    foundTextColor = true;
                }
                else if (line.rfind("bg_color=", 0) == 0)
                {
                    line = "bg_color=" + std::string(bgColor);
                    foundBgColor = true;
                }

                lines.push_back(line);
            }

            if (!foundTextColor)
            {
                lines.push_back("text_color=" + std::string(textColor));
            }

            if (!foundBgColor)
            {
                lines.push_back("bg_color=" + std::string(bgColor));
            }

            std::ofstream outputFile("config.txt", std::ios::trunc);
            if (outputFile)
            {
                for (const std::string& outputLine : lines)
                {
                    outputFile << outputLine << '\n';
                }
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
    // If the settings window is already open, bring it to the foreground instead
    // of creating a second instance.
    if (g_changeSettingsWindow != nullptr)
    {
        SetForegroundWindow(g_changeSettingsWindow);
        return;
    }

    // Register the window class only once during the application's lifetime.
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

    // Use the current cursor position as the initial window location.
    POINT cursorPosition;
    GetCursorPos(&cursorPosition);

    // Get the primary screen dimensions so the window can be kept visible.
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Define the desired dimensions of the settings window.
    const int windowWidth = 300;
    const int windowHeight = 200;

    int x = cursorPosition.x;
    int y = cursorPosition.y;

    // Move the window left if it extends past the right screen edge.
    if (x + windowWidth > screenWidth)
    {
        x = screenWidth - windowWidth - 10;
    }

    // Move the window up if it extends past the bottom screen edge.
    if (y + windowHeight > screenHeight)
    {
        y = screenHeight - windowHeight - 10;
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
        // Display the window before initializing its controls.
        ShowWindow(g_changeSettingsWindow, SW_SHOW);
        UpdateWindow(g_changeSettingsWindow);

        // Load the configured text color and populate its edit control.
        char textColorBuffer[256] = {};
        GetTextColorFromConfig(textColorBuffer, sizeof(textColorBuffer));
        SetWindowTextA(GetDlgItem(g_changeSettingsWindow, ID_TEXT_COLOR_EDIT), textColorBuffer);

        // Load the configured background color and populate its edit control.
        char bgColorBuffer[256] = {};
        GetBackgroundColorFromConfig(bgColorBuffer, sizeof(bgColorBuffer));
        SetWindowTextA(GetDlgItem(g_changeSettingsWindow, ID_BG_COLOR_EDIT), bgColorBuffer);
    }
}