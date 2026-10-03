// settingswindow.cpp
// Info window procedure for showing controls and current settings.

#include <windows.h>
#include <string>
#include <vector>
#include <winver.h>

#include "resource.h" // Instead of "resource_ids.h"
#include "readconfig.h"
#include "editsettings.h"

#pragma comment(lib, "Version.lib")

namespace
{
	// Tracks the settings window instance managed by the application.
	HWND g_changeSettingsWindow = nullptr;
}

// Retrieves the file version embedded in the running executable.
static std::wstring GetProgramVersion()
{
	wchar_t modulePath[MAX_PATH] = {};
	if (GetModuleFileNameW(nullptr, modulePath, _countof(modulePath)) == 0)
		return L"Unknown";

	DWORD dummy = 0;
	DWORD versionInfoSize = GetFileVersionInfoSizeW(modulePath, &dummy);
	if (versionInfoSize == 0)
		return L"Unknown";

	std::vector<BYTE> versionInfo(versionInfoSize);
	if (!GetFileVersionInfoW(modulePath, 0, versionInfoSize, versionInfo.data()))
		return L"Unknown";

	VS_FIXEDFILEINFO* fileInfo = nullptr;
	UINT fileInfoSize = 0;
	if (!VerQueryValueW(
		versionInfo.data(),
		L"\\",
		reinterpret_cast<LPVOID*>(&fileInfo),
		&fileInfoSize) ||
		fileInfo == nullptr)
	{
		return L"Unknown";
	}

	return std::to_wstring(HIWORD(fileInfo->dwFileVersionMS)) + L"." +
		std::to_wstring(LOWORD(fileInfo->dwFileVersionMS)) + L"." +
		std::to_wstring(HIWORD(fileInfo->dwFileVersionLS)) + L"." +
		std::to_wstring(LOWORD(fileInfo->dwFileVersionLS));
}

LRESULT CALLBACK InfoWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		// Create the button used to open the editable settings window.
		CreateWindowW(
			L"BUTTON",
			L"Edit Settings",
			WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			180, 40, 120, 25,
			hWnd,
			reinterpret_cast<HMENU>(ID_CHANGE_SETTINGS),
			GetModuleHandleW(nullptr),
			nullptr);

		// Create the button used to reset settings (delete config and create new).
		CreateWindowW(
			L"BUTTON",
			L"Reset Settings",
			WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			180, 75, 120, 25,
			hWnd,
			reinterpret_cast<HMENU>(ID_CHANGE_SETTINGS_RESET),
			GetModuleHandleW(nullptr),
			nullptr);

		// Create the close button (no title bar, just a close button).
		CreateWindowW(
			L"BUTTON",
			L"X",
			WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			280, 10, 20, 20,
			hWnd,
			reinterpret_cast<HMENU>(ID_CHANGE_SETTINGS_CLOSE),
			GetModuleHandleW(nullptr),
			nullptr);
		return 0;

	case WM_COMMAND:
		// Handle Edit Settings, Reset Settings, and Close buttons.
		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam))
			{
			case ID_CHANGE_SETTINGS:
				// Open the settings editor when the button is clicked.
				OpenChangeSettingsWindow(hWnd);
				break;

			case ID_CHANGE_SETTINGS_RESET:
				// Show confirmation dialog before resetting settings.
				if (MessageBoxW(
					hWnd,
					L"Are you sure you want to reset all settings? This will delete your current configuration and create a new one with default values.",
					L"Reset Settings",
					MB_YESNO | MB_ICONWARNING) == IDYES)
				{
					// Reset settings when the user confirms.
					// Delete existing config file if it exists.
					DeleteFileW(L"config.txt");

					// Create a new config file with default values.
					EnsureConfigFileExists();

					// Refresh the display to show the new settings.
					InvalidateRect(hWnd, nullptr, TRUE);
				}
				break;

			case ID_CHANGE_SETTINGS_CLOSE:
				// Close the window when the close button is clicked.
				DestroyWindow(hWnd);
				break;
			}
		}
		return DefWindowProc(hWnd, message, wParam, lParam);

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		RECT rect;
		GetClientRect(hWnd, &rect);

		// Paint the information window background black.
		HBRUSH hBg = CreateSolidBrush(RGB(0, 0, 0));
		FillRect(hdc, &rect, hBg);
		DeleteObject(hBg);

		// Configure white, transparent text so the black background remains visible.
		SetTextColor(hdc, RGB(255, 255, 255));
		SetBkMode(hdc, TRANSPARENT);

		// Read the current settings for display
		std::wstring wColor;
		ReadFromSettings(L"text_color", wColor);
		std::wstring wBgColor;
		ReadFromSettings(L"bg_color", wBgColor);

		// Build the help text and append the currently active settings.
		std::wstring info =
			L"\n\n\n\n\n\nControls:\n"
			L" - Drag window: Hold SHIFT and click & drag\n"
			L" - Double click: close window\n"
			L" - Right click: open this window (settings)\n"
			L"____________\n\nCurrent settings:\n";

		info += L"\nText Color: ";
		info += wColor;
		info += L"\nBackground Color: ";
		info += wBgColor;
		info += L"\n____________";
		info += L"\n\n\nMiniMonitor Version: ";
		info += GetProgramVersion();

		// Draw the text with word wrapping inside the client area.
		DrawTextW(
			hdc,
			info.c_str(),
			-1,
			&rect,
			DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_EXPANDTABS);

		EndPaint(hWnd, &ps);
		return 0;
	}

	case WM_CLOSE:
		// Destroy the window when the user closes it.
		DestroyWindow(hWnd);
		return 0;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			DestroyWindow(hWnd);
			return 0;
		}
		break;

	case WM_DESTROY:
		// No additional cleanup is currently required for this window.
		return 0;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

// Constants for the window's buttons
#define ID_CHANGE_SETTINGS_CLOSE 1001;