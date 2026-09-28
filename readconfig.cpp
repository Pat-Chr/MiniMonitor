// readconfig.cpp
// reads the config file and creates the necessary variables for the program to use
// Also creates the config file and gives it the standard values.

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>

    // Ensure config.txt exists at startup; if not, create with default entries.
    void EnsureConfigFileExists()
    {
        const char* name = "config.txt";
        char path[MAX_PATH] = {0};
        // Determine path to the executable's folder
        char mod[MAX_PATH] = {0};
        if (GetModuleFileNameA(NULL, mod, MAX_PATH) != 0) {
            char* p = strrchr(mod, '\\');
            if (p) {
                *++p = '\0'; // keep folder path including trailing '\\'
                strcpy_s(path, sizeof(path), mod);
                strcat_s(path, sizeof(path), name);
            } else {
                strcpy_s(path, sizeof(path), name);
            }
        } else {
            // Fallback: current working directory
            GetCurrentDirectoryA(MAX_PATH, path);
            size_t len = strlen(path);
            if (len && path[len-1] != '\\') strcat_s(path, sizeof(path), "\\");
            strcat_s(path, sizeof(path), name);
        }
        // Check existence
        DWORD attrs = GetFileAttributesA(path);
        if (attrs != INVALID_FILE_ATTRIBUTES) return; // already exists
        // Create (OPEN_ALWAYS = open or create)
        HANDLE h = CreateFileA(path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            DWORD err = GetLastError();
            char buf[256];
            sprintf_s(buf, "EnsureConfigFileExists: CreateFileA failed (err=%u) Path=%s\n", err, path);
            OutputDebugStringA(buf);
            return;
        }
        // if file was just created, write default content
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            const char* content =
                "# MiniMonitor configuration file\n"
                "# Key=Value\n"
                "text_color=0,255,100\n"
                "bg_color=0,0,0\n"
                "window_pos=100,100\n";
            DWORD written = 0;
            SetFilePointer(h, 0, NULL, FILE_BEGIN);
            WriteFile(h, content, (DWORD)strlen(content), &written, NULL);
        }

        CloseHandle(h);
    }

// Read from settings.
// Searches for the name of the setting, then returns the value.
// Input: "SettingName" Output: "SettingValue"
// Use like this:
// std::wstring wBgColor;
// ReadFromSettings(L"bg_color", wBgColor)
void ReadFromSettings(const std::wstring& SettingName, std::wstring& SettingValue)
{
    // Open config.txt for reading (use ANSI version since file is ASCII/UTF-8)
    const char* name = "config.txt";
    char path[MAX_PATH] = { 0 };
    
    // Determine path to the executable's folder
    char mod[MAX_PATH] = { 0 };
    if (GetModuleFileNameA(NULL, mod, MAX_PATH) != 0) {
        char* p = strrchr(mod, '\\');
        if (p) {
            *++p = '\0'; // keep folder path including trailing '\\'
            strcpy_s(path, sizeof(path), mod);
            strcat_s(path, sizeof(path), name);
        } else {
            strcpy_s(path, sizeof(path), name);
        }
    } else {
        // Fallback: current working directory
        GetCurrentDirectoryA(MAX_PATH, path);
        size_t len = strlen(path);
        if (len && path[len - 1] != '\\') strcat_s(path, sizeof(path), "\\");
        strcat_s(path, sizeof(path), name);
    }
    
    // Open the config file in binary mode to reliably detect BOM
    FILE* file = nullptr;
    if (fopen_s(&file, path, "rb") != 0 || !file) {
        return; // Config file not found or cannot be opened
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char* pLine = line;
        // Skip UTF-8 BOM if present
        if ((unsigned char)pLine[0] == 0xEF && (unsigned char)pLine[1] == 0xBB && (unsigned char)pLine[2] == 0xBF) {
            pLine += 3;
        }
        
        // Check if the line contains the setting name (case-insensitive comparison)
        std::string sLine(pLine);
        std::wstring wSettingName(SettingName);
        
        // Convert to wstring for comparison
        std::wstring wLine;
        for (size_t i = 0; i < sLine.length(); ++i) {
            wLine.push_back((wchar_t)sLine[i]);
        }
        
        if (wLine.find(wSettingName) != std::wstring::npos) {
            // Extract the value after the '=' sign or space
            char* delimiter = strchr(pLine, '=');
            if (!delimiter) {
                delimiter = strchr(pLine, ' ');
            }
            
            if (delimiter) {
                // Copy the value to a buffer
                std::wstring value;
                const char* valueStart = delimiter + 1;
                
                // Find end of value (newline or whitespace)
                while (*valueStart && *valueStart != '\n' && *valueStart != '\r') {
                    value.push_back((wchar_t)*valueStart);
                    valueStart++;
                }
                
                // Convert to wstring and set output parameter
                SettingValue = value;
                break;
            }
        }
    }
    
    fclose(file);
}