#include "framework.h"
#include "MiniMonitor.h"
#include <pdh.h>
#include <pdhmsg.h>
#include <string>
#include <vector>
#include <cmath>
#include <windows.h>
#include <cstring>

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
bool firstSampleTaken = false;

// Forward declarations of Windows functions and update function
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void UpdatePerformanceData();

// Info window procedure for showing controls / settings
LRESULT CALLBACK InfoWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rect;
        GetClientRect(hWnd, &rect);

        HBRUSH hBg = CreateSolidBrush(RGB(30, 30, 30));
        FillRect(hdc, &rect, hBg);
        DeleteObject(hBg);

        SetTextColor(hdc, RGB(200, 200, 200));
        SetBkMode(hdc, TRANSPARENT);

        LPCWSTR info =
            L"MiniMonitor Controls:\n\n"
            L" - Left click & drag: move window\n"
            L" - Double click: close window\n"
            L" - Right click: open this window (settings)\n\n"
            L"Settings will be implemented here.";

        DrawTextW(hdc, info, -1, &rect, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_EXPANDTABS);

        EndPaint(hWnd, &ps);
    }
    return 0;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        // Do not call PostQuitMessage here; this is a child/settings window
        return 0;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

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
        WS_EX_TOOLWINDOW,
        szWindowClass, L"",            // No title text (no help/info in title)
        WS_POPUP,
        100, 100, 70, 50, //dimensions
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

void UpdatePerformanceData()
{
    if (!firstSampleTaken) return;

    // Collect new data for both queries
    PdhCollectQueryData(cpuQuery);
    PdhCollectQueryData(gpuQuery);

    // 1. Get CPU Load
    PDH_FMT_COUNTERVALUE cpuVal;
    PDH_STATUS getStatus = PdhGetFormattedCounterValue(cpuCounter, PDH_FMT_DOUBLE, NULL, &cpuVal);
    if (getStatus == ERROR_SUCCESS && std::isfinite(cpuVal.doubleValue)) {
        cpuLoad = (float)cpuVal.doubleValue;
    } else {
        // optional: cpuLoad = 0 or error log
    }

    // 2. Get GPU Load (Summing all engines)
    // Because we used a wildcard, we use PdhGetFormattedCounterArray to get all engine values at once
    DWORD dwCount = 0;
    DWORD dwSize = 0;
    PPDH_FMT_COUNTERVALUE_ITEM_W pItems = nullptr;

    // First query: determines required byte-size (dwSize) and number of items (dwCount)
    PDH_STATUS status = PdhGetFormattedCounterArrayW(gpuCounter, PDH_FMT_DOUBLE, &dwSize, &dwCount, NULL);

    // PDH returns PDH_MORE_DATA when the buffer is too small — dwSize will then contain the required value
    if ((status == PDH_MORE_DATA || status == ERROR_SUCCESS) && dwSize > 0 && dwCount > 0) {
        pItems = (PPDH_FMT_COUNTERVALUE_ITEM_W)malloc(dwSize);
        if (pItems != nullptr) {
            status = PdhGetFormattedCounterArrayW(gpuCounter, PDH_FMT_DOUBLE, &dwSize, &dwCount, pItems);
            if (status == ERROR_SUCCESS) {
                float totalGpu = 0.0f;
                for (DWORD i = 0; i < dwCount; ++i) {
                    double val = pItems[i].FmtValue.doubleValue;
                    if (std::isfinite(val)) {
                        totalGpu += (float)val;
                    }
                }
                // Cap at 100% in case of weird multi-engine math
                gpuLoad = (totalGpu > 100.0f) ? 100.0f : totalGpu;
            }
            free(pItems);
            pItems = nullptr;
        }
    }
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

        // Draw Dark Background
        RECT rect;
        GetClientRect(hWnd, &rect);
        HBRUSH hBgBrush = CreateSolidBrush(RGB(20, 20, 20));
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
        swprintf_s(tmp, _countof(tmp), L"placeholder");
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
            SetTextColor(hdc, RGB(0, 255, 100));
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
            int width = 420, height = 260;
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
                WS_EX_OVERLAPPEDWINDOW,
                className,
                L"MiniMonitor - Settings",
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
