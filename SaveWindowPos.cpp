// file: SaveWindowPos.cpp
// Reads the window Position and stores it in the global variable WindowPos as a string in the format "left,top".
#include <windows.h>
#include <strsafe.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "saveasetting.h"

// Global function to save a setting to the configuration file. Can be used for any setting.
void SaveASetting(const std::wstring& key, const std::wstring& value);
// Example usage: SaveASetting(L"settingName", L"settingValue");

/**
 * Global buffer to store window position data.
 * Format: "left,top" (as a string)
 * Maximum size: 256 characters
 */
WCHAR WindowPos[256];

/**
 * Saves the current window position to a configuration file.
 * 
 * @param hwnd Handle to the window whose position is being saved.
 * 
 * @remarks
 * - Retrieves the window rectangle using GetWindowRect()
 * - Stores left and top coordinates in WindowPos buffer
 * 
 * @returns void
 */
void SaveWindowPos(HWND hwnd)
{
    // Buffer to hold the window's rectangular area (left, top, right, bottom)
    RECT windowRect{};

    // Attempt to retrieve the window rectangle from the system
    if (!GetWindowRect(hwnd, &windowRect))
    {
        // Failed to get window position - clear the global buffer
        WindowPos[0] = L'\0';
        return;
    }

    // Format the window position as a string in "left,top" format
    // Note: right and bottom coordinates are not yet included (future enhancement)
    StringCchPrintfW(
        WindowPos,
        _countof(WindowPos),
        L"%ld,%ld",
        windowRect.left,
        windowRect.top);

    // Use SaveASetting() from saveasetting.h
    SaveASetting(L"window_pos", WindowPos);
}
