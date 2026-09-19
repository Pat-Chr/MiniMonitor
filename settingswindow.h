#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize any resources required by the settings UI (icons, classes, etc.)
BOOL InitSettingsWindow(HINSTANCE hInstance);

// Create a modeless settings window. Returns the window handle or NULL on failure.
HWND CreateSettingsWindow(HWND hParent);

// Show a modal settings dialog. Returns IDOK, IDCANCEL or other dialog result.
INT_PTR ShowSettingsDialog(HWND hParent);

// Window/dialog procedure for the settings UI
INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Load/Save/Apply settings (implementation expected in settingswindow.cpp or similar)
void LoadSettings();
void SaveSettings();
void ApplySettings();

// Convenience: open or focus the settings window
void OpenSettingsWindow(HWND hParent);

#ifdef __cplusplus
}
#endif