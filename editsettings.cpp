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
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            50, 100, 60, 25,
            hWnd,
            reinterpret_cast<HMENU>(ID_SAVE),
            GetModuleHandleW(nullptr),
            nullptr);

        // Create Cancel button
        CreateWindowW(
            L"BUTTON",
            L"Cancel",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            180, 100, 60, 25,
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

            // Parse current text color
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

            // Parse current background color
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
        windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));

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
    const int windowWidth = 310;
    const int windowHeight = 210;

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
    }
}