#include "framework.h"
#include "MiniMonitor.h"
#include <pdh.h>
#include <pdhmsg.h>
#include <string>
#include <vector>
#include <cmath>
#include <windows.h>
#include <cstring>
#include "readconfig.h"
#include "SaveWindowPos.h"
#include "settingswindow.h"

#pragma comment(lib, "pdh.lib")

#define MAX_LOADSTRING 100

// Global instances and window text
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

// PDH performance counters for CPU and GPU
PDH_HQUERY cpuQuery;
PDH_HCOUNTER cpuCounter;
PDH_HQUERY gpuQuery;
PDH_HCOUNTER gpuCounter;

// Current load values and initialization status
float cpuLoad = 0.0f;
float gpuLoad = 0.0f;
float ramLoad = 0.0f;
bool firstSampleTaken = false;

// Forward declarations of update function
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    InfoWndProc(HWND, UINT, WPARAM, LPARAM);
void UpdatePerformanceData();

// Hide toolbar, rebar and statusbar child windows in the window
static BOOL CALLBACK HideTopChildren(HWND child, LPARAM)
{
    char cls[64] = {0};
    GetClassNameA(child, cls, sizeof(cls));
    if (strcmp(cls, "ToolbarWindow32") == 0 || strcmp(cls, "ReBarWindow32") == 0 || strcmp(cls, "msctls_statusbar32") == 0) {
        ShowWindow(child, SW_HIDE);
        return TRUE;
    }

    char txt[256] = {0};
    GetWindowTextA(child, txt, sizeof(txt));
    if (txt[0]) {
        // Space for additional logic when detecting visible text
    }
    return TRUE;
}

void DisableTopBar(HWND hwnd)
{
    if (!hwnd) return;

    // Remove the menu bar if present
    HMENU hMenu = GetMenu(hwnd);
    if (hMenu) {
        SetMenu(hwnd, NULL);
        DrawMenuBar(hwnd);
    }

    // Hide common toolbar/rebar/status/help child windows
    EnumChildWindows(hwnd, HideTopChildren, 0);

    // In case a custom control draws a top bar, try resizing client area (optional)
    // RECT r; if (GetClientRect(hwnd, &r)) { InvalidateRect(hwnd, NULL, TRUE); UpdateWindow(hwnd); }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Ensure configuration file exists at application start
    EnsureConfigFileExists();

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MINIMONITOR, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    // --- PDH INITIALIZATION ---
    PDH_STATUS status;
    status = PdhOpenQuery(NULL, NULL, &cpuQuery);
    if (status != ERROR_SUCCESS) { /* handle error */ }

    status = PdhAddEnglishCounterW(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuCounter);
    if (status != ERROR_SUCCESS) {
        // Log/handle – without a valid counter cpuLoad remains 0
    }

    PdhOpenQuery(NULL, NULL, &gpuQuery);
    // Use the wildcard to grab all GPU engines
    PdhAddCounterW(gpuQuery, L"\\GPU Engine(*)\\Utilization Percentage", NULL, &gpuCounter);

    // VERY IMPORTANT: The very first collection establishes the baseline.
    // We call it here so the first timer tick is actually the SECOND sample.
    PdhCollectQueryData(cpuQuery);
    PdhCollectQueryData(gpuQuery);
    firstSampleTaken = true;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MINIMONITOR));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MINIMONITOR);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    // WS_POPUP: no border/title
    // WS_EX_TOOLWINDOW: hide from taskbar
    // No WS_EX_TOPMOST -> window is not always-on-top
    HWND hWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        szWindowClass, L"",            // No title text (no help/info in title)
        WS_POPUP | WS_BORDER,
        100, 100, 80, 50, //dimensions
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;

    // Remove top bar elements (menu, toolbars, status bar)
    DisableTopBar(hWnd);

    // Set timer for 500ms
    SetTimer(hWnd, 1, 500, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_TIMER:
        UpdatePerformanceData();
        InvalidateRect(hWnd, NULL, FALSE); // Trigger a repaint
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        //fetch the background color from the config file
        char bg_Color[128] = {0};
        GetBackgroundColorFromConfig(bg_Color, sizeof(bg_Color));
        // Convert config text (ANSI) to a wide string for DrawTextW
        WCHAR wBgColor[128] = {0};
        MultiByteToWideChar(CP_ACP, 0, bg_Color, -1, wBgColor, _countof(wBgColor));

		// parse config "R, G, B" and apply as COLORREF
		int r = 0, g = 0, b = 0; // default black
		int parsed = sscanf_s(bg_Color, "%d , %d , %d", &r, &g, &b);
		if (parsed == 3) {
			if (r < 0) r = 0; if (r > 255) r = 255;
			if (g < 0) g = 0; if (g > 255) g = 255;
			if (b < 0) b = 0; if (b > 255) b = 255;
            SetBkColor(hdc, ((COLORREF)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(b)) << 16))));
		}
		else {
			// Fallback color in case of parsing failure
            SetBkColor(hdc, ((COLORREF)(((BYTE)(0) | ((WORD)((BYTE)(0)) << 8)) | (((DWORD)(BYTE)(0)) << 16))));
		}

		// Draw Background.
        RECT rect;
        GetClientRect(hWnd, &rect);
        HBRUSH hBgBrush = CreateSolidBrush(((COLORREF)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(b)) << 16))));
        FillRect(hdc, &rect, hBgBrush);
        DeleteObject(hBgBrush);

        // Prepare Text (stacked: flexible number of lines)
        // Add more lines by pushing to 'lines' below
        std::vector<std::wstring> lines;
        wchar_t tmp[128];

        // Existing lines
        swprintf_s(tmp, _countof(tmp), L"CPU:%0.0f%%", cpuLoad);
        lines.emplace_back(tmp);
        swprintf_s(tmp, _countof(tmp), L"GPU:%0.0f%%", gpuLoad);
        lines.emplace_back(tmp);
        swprintf_s(tmp, _countof(tmp), L"RAM:%0.0f%%", ramLoad);
        lines.emplace_back(tmp);

        // Example of adding another line (commented out) — uncomment or add more as needed
        // swprintf_s(tmp, _countof(tmp), L"RAM:%0.0f%%", ramLoad);
        // lines.emplace_back(tmp);

        // Count how many swprintf_s-produced lines we have
        int count = static_cast<int>(lines.size());
        int swprintfCount = count; // number of swprintf_s calls that produced lines here

        // Determine per-line rects and draw each centered in its slice
        if (count > 0)
        {
			// SetTextColor from config
            char colorBuffer[128] = { 0 };
            GetTextColorFromConfig(colorBuffer, sizeof(colorBuffer));
            WCHAR wColor[128] = { 0 };
            MultiByteToWideChar(CP_ACP, 0, colorBuffer, -1, wColor, _countof(wColor));

            // Parse config "R, G, B" and apply as COLORREF
            int r = 200, g = 200, b = 200; // default
            int parsed = sscanf_s(colorBuffer, "%d , %d , %d", &r, &g, &b);
            if (parsed == 3) {
                if (r < 0) r = 0; if (r > 255) r = 255;
                if (g < 0) g = 0; if (g > 255) g = 255;
                if (b < 0) b = 0; if (b > 255) b = 255;
                SetTextColor(hdc, ((COLORREF)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(b)) << 16))));
            } else {
                // Fallback color in case of parsing failure
                SetTextColor(hdc, ((COLORREF)(((BYTE)(200) | ((WORD)((BYTE)(200)) << 8)) | (((DWORD)(BYTE)(200)) << 16))));
            }
            SetBkMode(hdc, TRANSPARENT);

            // Measure one line height using current font
            TEXTMETRIC tm;
            GetTextMetrics(hdc, &tm);
            int lineHeight = tm.tmHeight;
            int padding = 8; // top+bottom padding

            int desiredClientHeight = lineHeight * count + padding * 2;

            // Compute non-client height so we resize the window correctly (client + non-client = window size)
            RECT wndRect;
            GetWindowRect(hWnd, &wndRect);
            int windowHeight = wndRect.bottom - wndRect.top;
            int clientHeight = rect.bottom - rect.top;
            int nonClientHeight = windowHeight - clientHeight;
            int desiredWindowHeight = desiredClientHeight + nonClientHeight;

            int currentWindowWidth = wndRect.right - wndRect.left;
            // Only resize if height differs (avoid flicker/continuous repaints)
            if (abs(windowHeight - desiredWindowHeight) > 1)
            {
                // Keep position, change size only
                SetWindowPos(hWnd, NULL, 0, 0, currentWindowWidth, desiredWindowHeight, SWP_NOMOVE | SWP_NOZORDER);
                // Update client rect after resize
                GetClientRect(hWnd, &rect);
                clientHeight = rect.bottom - rect.top;
            }

            int height = rect.bottom - rect.top;
            for (int i = 0; i < count; ++i)
            {
                RECT part = rect;
                part.top = rect.top + (height * i) / count;
                part.bottom = rect.top + (height * (i + 1)) / count;
                DrawTextW(hdc, lines[i].c_str(), -1, &part, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
        }

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_LBUTTONDOWN:
        // Allow dragging the borderless window by treating client clicks as caption drags
        ReleaseCapture();
        SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        break;

    case WM_LBUTTONDBLCLK:
        // Close window on double left-click

        // Save the last window position to the config file before exiting
        SaveWindowPos(hWnd);

        DestroyWindow(hWnd);
        break;

    case WM_RBUTTONUP:
    {

        // Open a modeless settings/info window
        const wchar_t* className = L"MiniMonitorSettings";
        HINSTANCE hInst = (HINSTANCE)GetModuleHandle(NULL);

        // Register class if not already registered
        WNDCLASSEX wcx = {};
        if (!GetClassInfoEx(hInst, className, &wcx))
        {
            wcx.cbSize = sizeof(WNDCLASSEX);
            wcx.style = CS_HREDRAW | CS_VREDRAW;
            wcx.lpfnWndProc = InfoWndProc;
            wcx.cbClsExtra = 0;
            wcx.cbWndExtra = 0;
            wcx.hInstance = hInst;
            wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
            wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
            wcx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wcx.lpszMenuName = NULL;
            wcx.lpszClassName = className;
            wcx.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
            RegisterClassEx(&wcx);
        }

        // If window already exists, bring it to front
        HWND infoHwnd = FindWindow(className, NULL);
        if (infoHwnd)
        {
            if (IsIconic(infoHwnd)) ShowWindow(infoHwnd, SW_RESTORE);
            ShowWindow(infoHwnd, SW_SHOWNORMAL);
            SetForegroundWindow(infoHwnd);
            SetFocus(infoHwnd);
        }
        else
        {
            // Position the settings window near the main window
            RECT rcMain = {};
            GetWindowRect(hWnd, &rcMain);
            int width = 320, height = 300;
            int x = rcMain.left + 40;
            int y = rcMain.top + 40;

            // Ensure the window is on-screen (basic clamp)
            int screenW = GetSystemMetrics(SM_CXSCREEN);
            int screenH = GetSystemMetrics(SM_CYSCREEN);
            if (x + width > screenW) x = screenW - width - 40;
            if (y + height > screenH) y = screenH - height - 40;
            if (x < 0) x = 40;
            if (y < 0) y = 40;

            HWND newHwnd = CreateWindowEx(
                WS_EX_TOPMOST,
                className,
                L"Settings",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                x, y, width, height,
                NULL,
                NULL,
                hInst,
                NULL);

            // Ensure the newly created window is shown and updated
            if (newHwnd)
            {
                ShowWindow(newHwnd, SW_SHOWNORMAL);
                UpdateWindow(newHwnd);
                SetForegroundWindow(newHwnd);
                SetFocus(newHwnd);
            }
        }
    }
    break;

    case WM_DESTROY:
        PdhCloseQuery(cpuQuery);
        PdhCloseQuery(gpuQuery);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
