// editsettings.h contains the declaration of the class EditSettings
#ifndef EDITSETTINGS_H
#define EDITSETTINGS_H

#include <windows.h>
#include <string>
#include <vector>

#pragma comment(lib, "Version.lib")

static std::wstring GetProgramVersion();

// Forward declaration for OpenChangeSettingsWindow
void OpenChangeSettingsWindow(HWND owner);

#endif // EDITSETTINGS_H

