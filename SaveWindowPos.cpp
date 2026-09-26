// file: SaveWindowPos.cpp
// Reads the window Position and stores it in the global variable WindowPos as a string in the format "left,top".
#include <windows.h>
#include <strsafe.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

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
 * - Reads existing config.txt, updates window_pos line with new values
 * - Writes updated content back to config.txt (truncating previous content)
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

    // Open existing configuration file for reading
    std::ifstream inputFile("config.txt");
    std::vector<std::string> lines;
    std::string line;
    bool foundWindowPos = false;

    // Read all lines from the configuration file
    while (std::getline(inputFile, line))
    {
        // Check if this line is the window position entry
        if (line.rfind("window_pos=", 0) == 0)
        {
            // Replace existing window_pos line with updated coordinates
            line = "window_pos=" +
                std::to_string(windowRect.left) + "," +
                std::to_string(windowRect.top);

            // Mark that we found and updated the window position entry
            foundWindowPos = true;
        }

        // Store all lines (including modified ones) for later writing
        lines.push_back(line);
    }

    // If window_pos line was not found, add a new one with current coordinates
    if (!foundWindowPos)
    {
        lines.push_back(
            "window_pos=" +
            std::to_string(windowRect.left) + "," +
            std::to_string(windowRect.top));
    }

    // Open configuration file for writing (truncates existing content)
    std::ofstream outputFile("config.txt", std::ios::trunc);
    if (!outputFile)
    {
        // Failed to open file for writing - cannot proceed
        return;
    }

    // Write all lines back to the configuration file
    for (const std::string& outputLine : lines)
    {
        outputFile << outputLine << '\n';
    }

    // Close files automatically when they go out of scope
}
