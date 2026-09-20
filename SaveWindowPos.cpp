// file: SaveWindowPos.cpp
// Reads the window Position and stores it in the global variable WindowPos as a string in the format "left,top,right,bottom".
// Not finished yet, but the function can already be called for testing purposes.
#include <windows.h>
#include <strsafe.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

WCHAR WindowPos[256];

void SaveWindowPos(HWND hwnd)
{
    RECT windowRect{};

    if (!GetWindowRect(hwnd, &windowRect))
    {
        WindowPos[0] = L'\0';
        return;
    }

    StringCchPrintfW(
        WindowPos,
        _countof(WindowPos),
        L"%ld,%ld",
        windowRect.left,
        windowRect.top);

    std::ifstream inputFile("config.txt");
    std::vector<std::string> lines;
    std::string line;
    bool foundWindowPos = false;

    while (std::getline(inputFile, line))
    {
        if (line.rfind("window_pos=", 0) == 0)
        {
            line = "window_pos=" +
                std::to_string(windowRect.left) + "," +
                std::to_string(windowRect.top);

            foundWindowPos = true;
        }

        lines.push_back(line);
    }

    if (!foundWindowPos)
    {
        lines.push_back(
            "window_pos=" +
            std::to_string(windowRect.left) + "," +
            std::to_string(windowRect.top));
    }

    std::ofstream outputFile("config.txt", std::ios::trunc);
    if (!outputFile)
    {
        return;
    }

    for (const std::string& outputLine : lines)
    {
        outputFile << outputLine << '\n';
    }
}