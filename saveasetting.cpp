// file: SaveASetting.cpp
// Generic function to save any setting to a configuration file.
#include <windows.h>
#include <strsafe.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "saveasetting.h"

/**
 * Saves a generic setting value to a configuration file.
 * 
 * @param settingName Name of the setting (key) to save.
 * @param settingValue Value of the setting to store.
 * 
 * @remarks
 * - Reads existing config.txt, updates or adds the specified setting line
 * - Supports any string value (including numeric values converted to strings)
 * - Writes updated content back to config.txt (truncating previous content)
 * - If setting doesn't exist, creates a new entry with the provided name and value. We may not need that in general, but it is a good fallback.
 * - -----
 * - Usage example inside the function: SaveASetting(L"settingName", L"settingValue");
 * - Include the header "saveasetting.h" to use this function in other parts of the application.
 * - don't forget the void SaveASetting(const std::wstring& key, const std::wstring& value);
 * - at the top of the file to declare the function for use in other files.
 * - -----
 * @returns void
 */
void SaveASetting(const std::wstring& settingName, const std::wstring& settingValue)
{
    // Open existing configuration file for reading
    std::ifstream inputFile(L"config.txt");
    std::vector<std::string> lines;
    std::string line;
    std::string narrowSettingName = std::string(settingName.begin(), settingName.end());
    bool foundSetting = false;

    // Read all lines from the configuration file
    while (std::getline(inputFile, line))
    {
        // Check if this line is the setting entry (case-insensitive name match)
        if (line.rfind(narrowSettingName, 0) == 0)
        {
            // Replace existing setting line with updated value
            std::wstring newLine = L"" + settingName + L"=" + settingValue;
            
            // Convert wstring to string for storage
            lines.push_back(std::string(newLine.begin(), newLine.end()));

            // Mark that we found and updated the setting entry
            foundSetting = true;
        }
        else
        {
            // Store all other lines unchanged
            lines.push_back(line);
        }
    }

    inputFile.close();

    // If setting line was not found, add a new one with current values
    if (!foundSetting)
    {
        std::wstring newLine = L"" + settingName + L"=" + settingValue;
        lines.push_back(std::string(newLine.begin(), newLine.end()));
    }

    // Open configuration file for writing (truncates existing content)
    std::ofstream outputFile(L"config.txt", std::ios::trunc);
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
