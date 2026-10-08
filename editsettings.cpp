// file: E:\Visual Studio Projects\repos\MiniMonitor\editsettings.cpp
// file: editsettings.cpp
// Edit settings window procedure for showing controls / settings

#include <windows.h>
#include <string>
#include <vector>
#include <winver.h>
#include <commdlg.h>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cwchar>

#pragma comment(lib, "Version.lib")
#pragma comment(lib, "comdlg32.lib")

#include "saveasetting.h"
#include "readconfig.h"

constexpr int ID_CHANGE_SETTINGS = 1001;
constexpr int ID_SAVE = 1002;
constexpr int ID_CANCEL = 1003;
constexpr int ID_CHANGE_SETTINGS_CLOSE = 1004;
constexpr int ID_TEXT_COLOR_EDIT = 1005;
constexpr int ID_BG_COLOR_EDIT = 1006;
constexpr int ID_TEXT_COLOR_PICKER = 1007;
constexpr int ID_BG_COLOR_PICKER = 1008;
constexpr int ID_SHOW_BORDER_CHECKBOX = 1009;
constexpr int ID_SHOW_CPU_LINE_CHECKBOX = 1010;
constexpr int ID_SHOW_GPU_LINE_CHECKBOX = 1011;
constexpr int ID_SHOW_RAM_LINE_CHECKBOX = 1012;
constexpr int ID_SHOW_VRAM_LINE_CHECKBOX = 1013;

struct CheckboxSetting
{
    int controlId;
    const wchar_t* key;
    const wchar_t* label;
    int y;
};

constexpr CheckboxSetting checkboxSettings[] = {
    { ID_SHOW_BORDER_CHECKBOX, L"ShowBorder", L"Show Border", 85 },
    { ID_SHOW_CPU_LINE_CHECKBOX, L"ShowCPULine", L"Show CPU Load", 110 },
    { ID_SHOW_GPU_LINE_CHECKBOX, L"ShowGPULine", L"Show GPU Load", 135 },
    { ID_SHOW_RAM_LINE_CHECKBOX, L"ShowRAMLine", L"Show RAM Load", 160 },
    { ID_SHOW_VRAM_LINE_CHECKBOX, L"ShowVRAMLine", L"Show VRAM Load", 185 }
};

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

static void ChooseColorForEdit(HWND window, int editId, COLORREF fallback)
{
    COLORREF customColors[16] = {
        RGB(255, 0, 0), RGB(0, 255, 0), RGB(0, 0, 255), RGB(255, 255, 0),
        RGB(255, 0, 255), RGB(0, 255, 255), RGB(255, 255, 255), RGB(192, 192, 192),
        RGB(128, 128, 128), RGB(0, 0, 0), RGB(255, 192, 192), RGB(192, 255, 192),
        RGB(192, 192, 255), RGB(255, 255, 192), RGB(255, 192, 255), RGB(192, 255, 255)
    };
    char currentColor[256] = {};
    GetWindowTextA(GetDlgItem(window, editId), currentColor, sizeof(currentColor));

    unsigned int red = 0, green = 0, blue = 0;
    if (currentColor[0] && sscanf_s(currentColor, "%u,%u,%u", &red, &green, &blue) == 3)
        fallback = RGB(static_cast<BYTE>(red), static_cast<BYTE>(green), static_cast<BYTE>(blue));

    CHOOSECOLOR color = {};
    color.lStructSize = sizeof(color);
    color.hwndOwner = window;
    color.lpCustColors = customColors;
    color.rgbResult = fallback;
    color.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColor(&color))
    {
        char rgb[32];
        sprintf_s(rgb, "%u,%u,%u",
            static_cast<unsigned int>(GetRValue(color.rgbResult)),
            static_cast<unsigned int>(GetGValue(color.rgbResult)),
            static_cast<unsigned int>(GetBValue(color.rgbResult)));
        SetWindowTextA(GetDlgItem(window, editId), rgb);
    }
}

void RefreshWindowBorder();

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
            10, 37, 120, 20,
            hWnd,
            reinterpret_cast<HMENU>(0),
            GetModuleHandleW(nullptr),
            nullptr);

        CreateWindowW(
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            150, 37, 100, 20,
            hWnd,
            reinterpret_cast<HMENU>(ID_TEXT_COLOR_EDIT),
            GetModuleHandleW(nullptr),
            nullptr);

        // Create Text Color picker button
        CreateWindowW(
            L"BUTTON",
            L"...",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            255, 37, 30, 20,
            hWnd,
            reinterpret_cast<HMENU>(ID_TEXT_COLOR_PICKER),
            GetModuleHandleW(nullptr),
            nullptr);

        // Create Background Color label and edit control
        CreateWindowW(
            L"STATIC",
            L"Background Color:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, 60, 120, 20,
            hWnd,
            reinterpret_cast<HMENU>(0),
            GetModuleHandleW(nullptr),
            nullptr);

        CreateWindowW(
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            150, 60, 100, 20,
            hWnd,
            reinterpret_cast<HMENU>(ID_BG_COLOR_EDIT),
            GetModuleHandleW(nullptr),
            nullptr);

        // Create Background Color picker button
        CreateWindowW(
            L"BUTTON",
            L"...",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            255, 60, 30, 20,
            hWnd,
            reinterpret_cast<HMENU>(ID_BG_COLOR_PICKER),
            GetModuleHandleW(nullptr),
            nullptr);

        for (const auto& setting : checkboxSettings)
            CreateWindowW(L"BUTTON", setting.label,
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                10, setting.y, 290, 20, hWnd,
                reinterpret_cast<HMENU>(setting.controlId), GetModuleHandleW(nullptr), nullptr);

        // Create Close button (no title bar, just a close button)
        CreateWindowW(
            L"BUTTON",
            L"X",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            264, 10, 20, 20,
            hWnd,
            reinterpret_cast<HMENU>(ID_CHANGE_SETTINGS_CLOSE),
            GetModuleHandleW(nullptr),
            nullptr);

        // Create Save button
        CreateWindowW(
            L"BUTTON",
            L"Save",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            50, 220, 60, 25,
            hWnd,
            reinterpret_cast<HMENU>(ID_SAVE),
            GetModuleHandleW(nullptr),
            nullptr);

        // Create Cancel button
        CreateWindowW(
            L"BUTTON",
            L"Cancel",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            180, 220, 60, 25,
            hWnd,
            reinterpret_cast<HMENU>(ID_CANCEL),
            GetModuleHandleW(nullptr),
            nullptr);

        return 0;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, message == WM_CTLCOLORSTATIC ? TRANSPARENT : OPAQUE);
        return reinterpret_cast<INT_PTR>(GetStockObject(BLACK_BRUSH));
    }

    case WM_DRAWITEM:
    {
        auto* drawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (drawItem->CtlType != ODT_BUTTON)
            break;

        FillRect(drawItem->hDC, &drawItem->rcItem, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        FrameRect(drawItem->hDC, &drawItem->rcItem, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));

        wchar_t buttonText[64] = {};
        GetWindowTextW(drawItem->hwndItem, buttonText, static_cast<int>(sizeof(buttonText) / sizeof(buttonText[0])));
        SetTextColor(drawItem->hDC, RGB(255, 255, 255));
        SetBkMode(drawItem->hDC, TRANSPARENT);
        DrawTextW(drawItem->hDC, buttonText, -1, &drawItem->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        if (drawItem->itemState & ODS_FOCUS)
        {
            RECT focusRect = drawItem->rcItem;
            InflateRect(&focusRect, -4, -4);
            DrawFocusRect(drawItem->hDC, &focusRect);
        }
        return TRUE;
    }

    case WM_COMMAND:
        for (const auto& setting : checkboxSettings)
            if (LOWORD(wParam) == setting.controlId)
                return 0;

        switch (LOWORD(wParam))
        {
        case ID_TEXT_COLOR_PICKER:
            ChooseColorForEdit(hWnd, ID_TEXT_COLOR_EDIT, RGB(0, 0, 0));
            return 0;

        case ID_BG_COLOR_PICKER:
            ChooseColorForEdit(hWnd, ID_BG_COLOR_EDIT, RGB(255, 255, 255));
            return 0;

        // Save button reads the chosen values and saves them using SaveASetting()
        case ID_SAVE:
        {
            wchar_t textColor[256] = {};
            GetWindowTextW(
                GetDlgItem(hWnd, ID_TEXT_COLOR_EDIT),
                textColor,
                sizeof(textColor) / sizeof(wchar_t));

            wchar_t bgColor[256] = {};
            GetWindowTextW(
                GetDlgItem(hWnd, ID_BG_COLOR_EDIT),
                bgColor,
                sizeof(bgColor) / sizeof(wchar_t));

            SaveASetting(L"text_color", textColor);
            SaveASetting(L"bg_color", bgColor);

            for (const auto& setting : checkboxSettings)
            {
                const bool checked = SendMessageW(
                    GetDlgItem(hWnd, setting.controlId), BM_GETCHECK, 0, 0) == BST_CHECKED;
                SaveASetting(setting.key, checked ? L"true" : L"false");
            }

            // Refresh the main window, then close settings window

            RefreshWindowBorder();
            DestroyWindow(hWnd);
            return 0;
        }
        case ID_CHANGE_SETTINGS_CLOSE:
            // Close button - destroy the window
            DestroyWindow(hWnd);
            return 0;
        case ID_CANCEL:
            DestroyWindow(hWnd);
            return 0;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(hWnd);
            return 0;
        }
        break;

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
        windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));

        RegisterClassW(&windowClass);
        classRegistered = true;
    }

    // Use the current cursor position as the initial window location.
    POINT cursorPosition;
    GetCursorPos(&cursorPosition);

    // Get the monitor under the cursor so the window can be kept visible there.
    HMONITOR monitor = MonitorFromPoint(cursorPosition, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo = {};
    monitorInfo.cbSize = sizeof(monitorInfo);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Define the desired dimensions of the settings window.
    const int windowWidth = 295;
    const int windowHeight = 260;

    int x = cursorPosition.x;
    int y = cursorPosition.y;

    RECT workArea = { 0, 0, screenWidth, screenHeight };
    if (GetMonitorInfoW(monitor, &monitorInfo))
    {
        workArea = monitorInfo.rcWork;
    }

    constexpr int margin = 10;
    const int minX = workArea.left + margin;
    const int minY = workArea.top + margin;
    const int maxX = workArea.right - windowWidth - margin;
    const int maxY = workArea.bottom - windowHeight - margin;
    x = min(max(x, minX), max(minX, maxX));
    y = min(max(y, minY), max(minY, maxY));

    g_changeSettingsWindow = CreateWindowExW(
        0,
        L"MiniMonitorChangeSettingsWindow",
        L"",
        WS_POPUP | WS_BORDER,
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
        std::wstring wTextColor;
        ReadFromSettings(L"text_color", wTextColor);
        char textColorBuffer[256] = {};
        int lentext = static_cast<int>(wTextColor.size());
        if (lentext >= 255) {
            lentext = 254;
        }
        int convertedLentext = WideCharToMultiByte(CP_ACP, 0, wTextColor.c_str(), -1,
            textColorBuffer, 256, NULL, NULL);
        textColorBuffer[convertedLentext] = '\0';
        SetWindowTextA(GetDlgItem(g_changeSettingsWindow, ID_TEXT_COLOR_EDIT), textColorBuffer);

        // Load the configured background color and populate its edit control.
        std::wstring wBgColor;
        ReadFromSettings(L"bg_color", wBgColor);
        char bgColorBuffer[256] = {};
        int len = static_cast<int>(wBgColor.size());
        if (len >= 255) {
            len = 254;
        }
        int convertedLen = WideCharToMultiByte(CP_ACP, 0, wBgColor.c_str(), -1,
            bgColorBuffer, 256, NULL, NULL);
        bgColorBuffer[convertedLen] = '\0';
        SetWindowTextA(GetDlgItem(g_changeSettingsWindow, ID_BG_COLOR_EDIT), bgColorBuffer);

        for (const auto& setting : checkboxSettings)
        {
            std::wstring value;
            ReadFromSettings(setting.key, value);
            SendMessageW(GetDlgItem(g_changeSettingsWindow, setting.controlId), BM_SETCHECK,
                value == L"true" ? BST_CHECKED : BST_UNCHECKED, 0);
        }
    }
}