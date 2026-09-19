// readconfig.cpp
// reads the config file and creates the necessary variables for the program to use

#include <windows.h>
#include <cstdio>
#include <cstring>

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
            *++p = '\0'; // keep folder path including trailing '\'
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
            "bg_color=0,0,0\n";
        DWORD written = 0;
        SetFilePointer(h, 0, NULL, FILE_BEGIN);
        WriteFile(h, content, (DWORD)strlen(content), &written, NULL);
    }

    CloseHandle(h);
}

//get the text color from the config file
void GetTextColorFromConfig(char* colorBuffer, size_t bufferSize)
{
    const char* name = "config.txt";
    char path[MAX_PATH] = { 0 };
    // Determine path to the executable's folder
    char mod[MAX_PATH] = { 0 };
    if (GetModuleFileNameA(NULL, mod, MAX_PATH) != 0) {
        char* p = strrchr(mod, '\\');
        if (p) {
            *++p = '\0'; // keep folder path including trailing '\'
            strcpy_s(path, sizeof(path), mod);
            strcat_s(path, sizeof(path), name);
        }
        else {
            // No path separator found; use filename in current dir
            strcpy_s(path, sizeof(path), name);
        }
    }
    else {
        // Fallback: use current working directory
        GetCurrentDirectoryA(MAX_PATH, path);
        size_t len = strlen(path);
        if (len && path[len - 1] != '\\') strcat_s(path, sizeof(path), "\\");
        strcat_s(path, sizeof(path), name);
    }
    // Open the config file in binary mode to reliably detect BOM
    FILE* file = nullptr;
    if (fopen_s(&file, path, "rb") != 0 || !file) {
        OutputDebugStringA("GetTextColorFromConfig: could not open config.txt\n");
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char* pLine = line;
        // Skip UTF-8 BOM if present
        if ((unsigned char)pLine[0] == 0xEF && (unsigned char)pLine[1] == 0xBB && (unsigned char)pLine[2] == 0xBF) {
            pLine += 3;
        }
        // Look for "text_color=" prefix and copy the value
        if (strncmp(pLine, "text_color=", 11) == 0) {
            strncpy_s(colorBuffer, bufferSize, pLine + 11, _TRUNCATE);
            // Trim trailing CR/LF characters from the value
            size_t len = strlen(colorBuffer);
            while (len > 0 && (colorBuffer[len - 1] == '\n' || colorBuffer[len - 1] == '\r')) {
                colorBuffer[len - 1] = '\0';
                len--;
            }
            break;
        }
    }
    fclose(file);
}

// this is for later use, but for now it is not used
// Get the background color from config.txt
void GetBackgroundColorFromConfig(char* colorBuffer, size_t bufferSize)
{
    const char* name = "config.txt";
    char path[MAX_PATH] = { 0 };
    // Determine path to the executable's folder
    char mod[MAX_PATH] = { 0 };
    if (GetModuleFileNameA(NULL, mod, MAX_PATH) != 0) {
        char* p = strrchr(mod, '\\');
        if (p) {
            *++p = '\0'; // keep folder path including trailing '\'
            strcpy_s(path, sizeof(path), mod);
            strcat_s(path, sizeof(path), name);
        }
        else {
            // No path separator found; use filename in current dir
            strcpy_s(path, sizeof(path), name);
        }
    }
    else {
        // Fallback: use current working directory
        GetCurrentDirectoryA(MAX_PATH, path);
        size_t len = strlen(path);
        if (len && path[len - 1] != '\\') strcat_s(path, sizeof(path), "\\");
        strcat_s(path, sizeof(path), name);
    }
    // Open the config file in binary mode to reliably detect BOM
    FILE* file = nullptr;
    if (fopen_s(&file, path, "rb") != 0 || !file) {
        OutputDebugStringA("GetbackgroundColorFromConfig: could not open config.txt\n");
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char* pLine = line;
        // Skip UTF-8 BOM if present
        if ((unsigned char)pLine[0] == 0xEF && (unsigned char)pLine[1] == 0xBB && (unsigned char)pLine[2] == 0xBF) {
            pLine += 3;
        }
        // Look for "bg_color=" prefix and copy the value
        if (strncmp(pLine, "bg_color=", 9) == 0) {
            strncpy_s(colorBuffer, bufferSize, pLine + 9, _TRUNCATE);
            // Trim trailing CR/LF characters from the value
            size_t len = strlen(colorBuffer);
            while (len > 0 && (colorBuffer[len - 1] == '\n' || colorBuffer[len - 1] == '\r')) {
                colorBuffer[len - 1] = '\0';
                len--;
            }
            break;
        }
    }
    fclose(file);
}