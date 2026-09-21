// editsettings.h
#ifndef EDITSETTINGS_H
#define EDITSETTINGS_H

#include <windows.h>
#include <string>
#include <vector>

#pragma comment(lib, "Version.lib")

//fetch the variables from the config file
void GetTextColorFromConfig(char* buffer, size_t size);
void GetBackgroundColorFromConfig(char* buffer, size_t size);

static std::wstring GetProgramVersion();

// Forward declaration for OpenChangeSettingsWindow
void OpenChangeSettingsWindow(HWND owner);

#endif // EDITSETTINGS_H

