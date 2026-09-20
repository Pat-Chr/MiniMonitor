// Reads the window Position and stores it in the global variable WindowPos as a string in the format "left,top,right,bottom".
// Not finished yet, but the function can already be called for testing purposes.
#include <windows.h>
#include <strsafe.h>

WCHAR WindowPos[256];

void SaveWindowPos(HWND hwnd)
{
    RECT windowRect{};

    if (!GetWindowRect(hwnd, &windowRect))
    {
        WindowPos[0] = L'\0';
        return;
    }

    // Format: left,top
    StringCchPrintfW(
        WindowPos,
        _countof(WindowPos),
        L"%ld,%ld",
        windowRect.left,
        windowRect.top);

    // For testing purposes, display the window position in a message box
//    MessageBoxW(
//        nullptr,
//        WindowPos,
//        L"Testing WindowPos Variable",
//        MB_OK);
}