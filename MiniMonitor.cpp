#include "framework.h"
#include "MiniMonitor.h"
#include <pdh.h>
#include <string>
#include <vector>
#include <windows.h>
#include <cstring>
#include <cstdint>
#include "readconfig.h"
#include "SaveWindowPos.h"

#pragma comment(lib, "pdh.lib")

#define MAX_LOADSTRING 100
constexpr UINT WM_RESIZE_MONITOR = WM_APP + 1;
constexpr BYTE TRANSPARENT_HIT_ALPHA = 1;

// Global instances and window text
HINSTANCE hInst;
HWND g_mainWindow = nullptr;
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
float gpuRamLoad = 0.0f;
bool firstSampleTaken = false;

// Forward declarations of update function
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    InfoWndProc(HWND, UINT, WPARAM, LPARAM);
void UpdatePerformanceData();

static void ApplyWindowAppearance(HWND hWnd, bool isTransparent)
{
    std::wstring showBorderSetting = L"true";
    ReadFromSettings(L"ShowBorder", showBorderSetting);
    const bool showBorder = !isTransparent && (_wcsicmp(showBorderSetting.c_str(), L"true") == 0);

    const LONG_PTR currentStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
    const LONG_PTR newStyle = showBorder ? currentStyle | WS_BORDER : currentStyle & ~static_cast<LONG_PTR>(WS_BORDER);
    const LONG_PTR currentExStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
    const LONG_PTR newExStyle = isTransparent ? currentExStyle | WS_EX_LAYERED : currentExStyle & ~static_cast<LONG_PTR>(WS_EX_LAYERED);

    if (newStyle != currentStyle || newExStyle != currentExStyle) {
        SetWindowLongPtr(hWnd, GWL_STYLE, newStyle);
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, newExStyle);
        SetWindowPos(hWnd, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

}

void RefreshWindowBorder()
{
    if (!g_mainWindow) return;

    std::wstring bgTransparent;
    ReadFromSettings(L"bg_transparent", bgTransparent);
    const bool isTransparent = bgTransparent.empty() || (_wcsicmp(bgTransparent.c_str(), L"true") == 0);
    ApplyWindowAppearance(g_mainWindow, isTransparent);

    InvalidateRect(g_mainWindow, NULL, TRUE);
    UpdateWindow(g_mainWindow);
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

// Read the window position from settings, defaulting to (100, 100) if not found.
static POINT ReadWindowPosition()
{
    constexpr POINT defaultPosition{ 100, 100 };
    constexpr LONG windowWidth = 80;
    constexpr LONG windowHeight = 10;

    // Read window position as string from the settings and parse "x,y"
    std::wstring posStr;
    ReadFromSettings(L"window_pos", posStr);

    POINT position = defaultPosition;
    if (!posStr.empty()) {
        LONG px = defaultPosition.x;
        LONG py = defaultPosition.y;
        int parsed = swscanf_s(posStr.c_str(), L"%ld , %ld", &px, &py);
        if (parsed == 2) {
            position.x = px;
            position.y = py;
        }
    }

    RECT windowRect{
        position.x,
        position.y,
        position.x + windowWidth,
        position.y + windowHeight
    };

    HMONITOR monitor = MonitorFromRect(
        &windowRect,
        MONITOR_DEFAULTTONULL);

    if (!monitor)
        return defaultPosition;

    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);

    if (!GetMonitorInfoW(monitor, &monitorInfo))
        return defaultPosition;

    const RECT& screenRect = monitorInfo.rcMonitor;

    const bool fullyVisible =
        windowRect.left >= screenRect.left &&
        windowRect.top >= screenRect.top &&
        windowRect.right <= screenRect.right &&
        windowRect.bottom <= screenRect.bottom;

    return fullyVisible ? position : defaultPosition;
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

    const POINT windowPosition = ReadWindowPosition();


    // Config option. Show border or no border. Default is true.
    std::wstring ShowBorder = L"true";
    ReadFromSettings(L"ShowBorder", ShowBorder);
    // Convert wstring to bool (handles "true"/"false", "1"/"0", etc.)
    bool showBorder = (_wcsicmp(ShowBorder.c_str(), L"true") == 0);

    // WS_POPUP: no border/title
    // WS_EX_TOOLWINDOW: hide from taskbar
    // WS_EX_TOPMOST: keep the monitor window above other windows
    HWND hWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        szWindowClass, L"",
        WS_POPUP | (showBorder ? WS_BORDER : 0),
        windowPosition.x, windowPosition.y, 85, 10,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
        return FALSE;

    g_mainWindow = hWnd;

    // Remove top bar elements (menu, toolbars, status bar)
    DisableTopBar(hWnd);

    // Set timer for 200ms
    // Planned feature. timer options. e.g., 100ms, 200ms, 500ms, etc.
    // testing needed to ensure the timer is not too frequent or too slow.
    SetTimer(hWnd, 1, 200, NULL);

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

    case WM_RESIZE_MONITOR:
    {
        RECT wndRect;
        if (GetWindowRect(hWnd, &wndRect)) {
            const int currentHeight = wndRect.bottom - wndRect.top;
            const int currentWidth = wndRect.right - wndRect.left;
            const int desiredHeight = static_cast<int>(wParam);
            if (abs(currentHeight - desiredHeight) > 1) {
                SetWindowPos(hWnd, NULL, 0, 0, currentWidth, desiredHeight,
                    SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
    }
    break;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rect;
        GetClientRect(hWnd, &rect);

        HDC drawingHdc = hdc;
        HDC memoryDc = CreateCompatibleDC(hdc);
        HBITMAP memoryBitmap = nullptr;
        HGDIOBJ previousBitmap = nullptr;
        void* bitmapPixels = nullptr;
        bool useDoubleBuffer = false;

        const int clientWidth = rect.right - rect.left;
        const int clientHeight = rect.bottom - rect.top;
        if (memoryDc && clientWidth > 0 && clientHeight > 0) {
            BITMAPINFO bitmapInfo{};
            bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bitmapInfo.bmiHeader.biWidth = clientWidth;
            bitmapInfo.bmiHeader.biHeight = -clientHeight;
            bitmapInfo.bmiHeader.biPlanes = 1;
            bitmapInfo.bmiHeader.biBitCount = 32;
            bitmapInfo.bmiHeader.biCompression = BI_RGB;
            memoryBitmap = CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, &bitmapPixels, nullptr, 0);
            if (memoryBitmap) {
                previousBitmap = SelectObject(memoryDc, memoryBitmap);
                if (previousBitmap && previousBitmap != HGDI_ERROR) {
                    drawingHdc = memoryDc;
                    useDoubleBuffer = true;
                }
            }
        }

        int r = 0, g = 0, b = 0; // declare here - they must be in the same scope

        // fetch the background color from the config file
        char bg_Color[128] = { 0 };
        std::wstring readBgColor;
        ReadFromSettings(L"bg_color", readBgColor);

        // Convert wstring to char array (ANSI)
        int result = WideCharToMultiByte(CP_ACP, 0, readBgColor.c_str(), -1,
            bg_Color, _countof(bg_Color), NULL, NULL);

        if (result == 0) {
            // Conversion failed, use default color
            SetBkColor(drawingHdc, RGB(0, 0, 0));
        }
        else {
            // parse config "R, G, B" and apply as COLORREF

            if (sscanf_s(bg_Color, "%d , %d , %d", &r, &g, &b) == 3) {
                if (r < 0) r = 0; if (r > 255) r = 255;
                if (g < 0) g = 0; if (g > 255) g = 255;
                if (b < 0) b = 0; if (b > 255) b = 255;

                SetBkColor(drawingHdc, RGB(r, g, b));
            }
            else {
                // Fallback color in case of parsing failure
                SetBkColor(drawingHdc, RGB(0, 0, 0));
            }
        }

        std::wstring bgTransparent;
        ReadFromSettings(L"bg_transparent", bgTransparent);
        const bool isTransparent = bgTransparent.empty() || (_wcsicmp(bgTransparent.c_str(), L"true") == 0);

        ApplyWindowAppearance(hWnd, isTransparent);

        COLORREF textColor = RGB(200, 200, 200);
        std::wstring wTextColor;
        ReadFromSettings(L"text_color", wTextColor);
        char textColorBuffer[256] = {};
        const int convertedTextColor = WideCharToMultiByte(CP_ACP, 0, wTextColor.c_str(), -1,
            textColorBuffer, _countof(textColorBuffer), NULL, NULL);
        int textR = 200, textG = 200, textB = 200;
        if (convertedTextColor > 0 && sscanf_s(textColorBuffer, "%d , %d , %d", &textR, &textG, &textB) == 3) {
            textR = min(max(textR, 0), 255);
            textG = min(max(textG, 0), 255);
            textB = min(max(textB, 0), 255);
            textColor = RGB(textR, textG, textB);
        }

        COLORREF transparentRenderColor = RGB(255, 0, 255);
        if (isTransparent) {
            const COLORREF candidates[] = {
                RGB(255, 0, 255), RGB(0, 255, 255), RGB(255, 255, 0),
                RGB(0, 0, 255), RGB(0, 255, 0), RGB(255, 0, 0), RGB(0, 0, 0), RGB(255, 255, 255)
            };
            int greatestDistance = -1;
            for (COLORREF candidate : candidates) {
                const int redDistance = static_cast<int>(GetRValue(candidate)) - GetRValue(textColor);
                const int greenDistance = static_cast<int>(GetGValue(candidate)) - GetGValue(textColor);
                const int blueDistance = static_cast<int>(GetBValue(candidate)) - GetBValue(textColor);
                const int distance = redDistance * redDistance + greenDistance * greenDistance + blueDistance * blueDistance;
                if (distance > greatestDistance) {
                    greatestDistance = distance;
                    transparentRenderColor = candidate;
                }
            }

            HBRUSH hBgBrush = CreateSolidBrush(transparentRenderColor);
            FillRect(drawingHdc, &rect, hBgBrush);
            DeleteObject(hBgBrush);
            SetBkMode(drawingHdc, TRANSPARENT);
        }
        else {
            HBRUSH hBgBrush = CreateSolidBrush(RGB(r, g, b));
            FillRect(drawingHdc, &rect, hBgBrush);
            DeleteObject(hBgBrush);
        }

        // Prepare Text (stacked: flexible number of lines)
        // Add more lines by pushing to 'lines' below
        std::vector<std::wstring> lines;
        wchar_t tmp[128];

        // Reading the setting if the line should be shown or not.
        //CPU
        std::wstring StringShowCPULine = L"true";
        ReadFromSettings(L"ShowCPULine", StringShowCPULine);
        bool ShowCPULine = (_wcsicmp(StringShowCPULine.c_str(), L"true") == 0);
        //GPU
        std::wstring StringShowGPULine = L"true";
        ReadFromSettings(L"ShowGPULine", StringShowGPULine);
        bool ShowGPULine = (_wcsicmp(StringShowGPULine.c_str(), L"true") == 0);
        //RAM
        std::wstring StringShowRAMLine = L"true";
        ReadFromSettings(L"ShowRAMLine", StringShowRAMLine);
        bool ShowRAMLine = (_wcsicmp(StringShowRAMLine.c_str(), L"true") == 0);
        //VRAM
        std::wstring StringShowVRAMLine = L"true";
        ReadFromSettings(L"ShowVRAMLine", StringShowVRAMLine);
        bool ShowVRAMLine = (_wcsicmp(StringShowVRAMLine.c_str(), L"true") == 0);

		// Adding lines one by one, using swprintf_s to format the text
        // If thhe line is turned off, it will not be drawn.
        if (ShowCPULine) {
            swprintf_s(tmp, _countof(tmp), L"CPU:%3.0f%%", cpuLoad);
            lines.emplace_back(tmp);
        }
        if (ShowGPULine) {
            swprintf_s(tmp, _countof(tmp), L"GPU:%3.0f%%", gpuLoad);
            lines.emplace_back(tmp);
        }
        if (ShowRAMLine) {
            swprintf_s(tmp, _countof(tmp), L"RAM:%3.0f%%", ramLoad);
            lines.emplace_back(tmp);
        }
        if (ShowVRAMLine) {
            swprintf_s(tmp, _countof(tmp), L"VRAM:%3.0f%%", gpuRamLoad);
            lines.emplace_back(tmp);
        }

        // Example of adding another line (commented out) — uncomment or add more as needed
        // swprintf_s(tmp, _countof(tmp), L"RAM:%0.0f%%", ramLoad);
        // lines.emplace_back(tmp);

        // Count how many swprintf_s-produced lines we have
        int count = static_cast<int>(lines.size());
        int swprintfCount = count; // number of swprintf_s calls that produced lines here

        // get the text color from the config file
        if (count > 0)
        {
            SetTextColor(drawingHdc, textColor);
            if (isTransparent)
                SetBkMode(drawingHdc, TRANSPARENT);

            // Measure one line height using current font
            TEXTMETRIC tm;
            GetTextMetrics(drawingHdc, &tm);
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

            // Only resize if height differs (avoid flicker/continuous repaints)
            if (abs(windowHeight - desiredWindowHeight) > 1)
            {
                PostMessage(hWnd, WM_RESIZE_MONITOR,
                    static_cast<WPARAM>(desiredWindowHeight), 0);
            }

            int height = rect.bottom - rect.top;
            for (int i = 0; i < count; ++i)
            {
                RECT part = rect;
                part.top = rect.top + (height * i) / count;
                part.bottom = rect.top + (height * (i + 1)) / count;
                DrawTextW(drawingHdc, lines[i].c_str(), -1, &part, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
        }

        if (useDoubleBuffer && isTransparent && bitmapPixels) {
            auto* pixels = static_cast<std::uint32_t*>(bitmapPixels);
            const int backgroundRed = GetRValue(transparentRenderColor);
            const int backgroundGreen = GetGValue(transparentRenderColor);
            const int backgroundBlue = GetBValue(transparentRenderColor);
            const int foregroundRed = GetRValue(textColor);
            const int foregroundGreen = GetGValue(textColor);
            const int foregroundBlue = GetBValue(textColor);

            for (int i = 0; i < clientWidth * clientHeight; ++i) {
                const int sourceBlue = static_cast<int>(pixels[i] & 0xFF);
                const int sourceGreen = static_cast<int>((pixels[i] >> 8) & 0xFF);
                const int sourceRed = static_cast<int>((pixels[i] >> 16) & 0xFF);

                double coverage = 0.0;
                int channelCount = 0;
                const auto addCoverage = [&coverage, &channelCount](int source, int background, int foreground) {
                    if (foreground != background) {
                        double channelCoverage = static_cast<double>(source - background) / (foreground - background);
                        if (channelCoverage < 0.0) channelCoverage = 0.0;
                        if (channelCoverage > 1.0) channelCoverage = 1.0;
                        coverage += channelCoverage;
                        ++channelCount;
                    }
                };

                addCoverage(sourceRed, backgroundRed, foregroundRed);
                addCoverage(sourceGreen, backgroundGreen, foregroundGreen);
                addCoverage(sourceBlue, backgroundBlue, foregroundBlue);
                coverage = channelCount > 0 ? coverage / channelCount : 0.0;

                int alphaValue = static_cast<int>(coverage * 255.0 + 0.5);
                if (alphaValue < TRANSPARENT_HIT_ALPHA) alphaValue = TRANSPARENT_HIT_ALPHA;
                if (alphaValue > 255) alphaValue = 255;
                const BYTE alpha = static_cast<BYTE>(alphaValue);
                const BYTE red = static_cast<BYTE>((foregroundRed * alpha + 127) / 255);
                const BYTE green = static_cast<BYTE>((foregroundGreen * alpha + 127) / 255);
                const BYTE blue = static_cast<BYTE>((foregroundBlue * alpha + 127) / 255);
                pixels[i] = (static_cast<std::uint32_t>(alpha) << 24) |
                    (static_cast<std::uint32_t>(red) << 16) |
                    (static_cast<std::uint32_t>(green) << 8) | blue;
            }

            RECT windowRect{};
            GetWindowRect(hWnd, &windowRect);
            POINT destination{ windowRect.left, windowRect.top };
            POINT source{ 0, 0 };
            SIZE size{ clientWidth, clientHeight };
            HDC screenDc = GetDC(nullptr);
            BLENDFUNCTION blend{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
            UpdateLayeredWindow(hWnd, screenDc, &destination, &size, memoryDc, &source,
                0, &blend, ULW_ALPHA);
            ReleaseDC(nullptr, screenDc);
        }
        else if (useDoubleBuffer) {
            BitBlt(hdc, 0, 0, clientWidth, clientHeight, memoryDc, 0, 0, SRCCOPY);
        }

        EndPaint(hWnd, &ps);
        if (previousBitmap && previousBitmap != HGDI_ERROR) {
            SelectObject(memoryDc, previousBitmap);
        }
        if (memoryBitmap) {
            DeleteObject(memoryBitmap);
        }
        if (memoryDc) {
            DeleteDC(memoryDc);
        }
    }
    break;

    case WM_LBUTTONDOWN:
        // Only allow dragging if SHIFT is held down to prevent accidental moves
        if (GetKeyState(VK_SHIFT) & 0x8000) {
            ReleaseCapture();
            SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }
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
            int width = 310, height = 350;
            int x = rcMain.left + 40;
            int y = rcMain.top + 40;

            HMONITOR monitor = MonitorFromRect(&rcMain, MONITOR_DEFAULTTONEAREST);
            MONITORINFO monitorInfo = {};
            monitorInfo.cbSize = sizeof(monitorInfo);
            if (GetMonitorInfoW(monitor, &monitorInfo))
            {
                const RECT& workArea = monitorInfo.rcWork;
                constexpr int margin = 40;
                const int minX = workArea.left + margin;
                const int minY = workArea.top + margin;
                const int maxX = workArea.right - width - margin;
                const int maxY = workArea.bottom - height - margin;
                x = min(max(x, minX), max(minX, maxX));
                y = min(max(y, minY), max(minY, maxY));
            }

            HWND newHwnd = CreateWindowEx(
                WS_EX_TOPMOST,
                className,
                L"Settings",
                WS_POPUP | WS_BORDER,
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

    // Keep the window within the virtual desktop while allowing it to cross monitor boundaries.
    case WM_MOVING:
    {
        RECT* windowRect = reinterpret_cast<RECT*>(lParam);
        if (windowRect != nullptr)
        {
            const int virtualLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
            const int virtualTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
            const int virtualRight = virtualLeft + GetSystemMetrics(SM_CXVIRTUALSCREEN);
            const int virtualBottom = virtualTop + GetSystemMetrics(SM_CYVIRTUALSCREEN);
            const int windowWidth = windowRect->right - windowRect->left;
            const int windowHeight = windowRect->bottom - windowRect->top;
            constexpr int margin = 5;

            const int minLeft = virtualLeft + margin;
            const int maxLeft = max(minLeft, virtualRight - windowWidth - margin);
            const int minTop = virtualTop + margin;
            const int maxTop = max(minTop, virtualBottom - windowHeight - margin);

            windowRect->left = min(max(windowRect->left, minLeft), maxLeft);
            windowRect->right = windowRect->left + windowWidth;
            windowRect->top = min(max(windowRect->top, minTop), maxTop);
            windowRect->bottom = windowRect->top + windowHeight;
        }

        return TRUE;
    }

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
