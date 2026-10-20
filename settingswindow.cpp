// file: settingswindow.cpp
// settingswindow.cpp
// Implements the InfoWndProc window procedure for the Settings Window.
// Handles WM_CREATE, WM_COMMAND, WM_PAINT, WM_CLOSE, and WM_KEYDOWN messages.
// Provides buttons to edit/reset/close.

#include <windows.h>
#include <string>
#include <vector>
#include <winver.h>
#include <shellapi.h>

#include "resource.h" // Instead of "resource_ids.h"
#include "readconfig.h"
#include "editsettings.h"

#pragma comment(lib, "Version.lib")

namespace
{
	constexpr wchar_t kProjectUrl[] = L"https://github.com/Pat-Chr/MiniMonitor";

	// Tracks the settings window instance managed by the application.
	HWND g_changeSettingsWindow = nullptr;

	// Flag to track if dragging is active
	bool g_isDragging = false;

	// Store original mouse position for drag offset
	POINT g_dragOffset = { 0, 0 };

	// Clickable bounds for the project URL in the help text.
	RECT g_linkRect = { 0, 0, 0, 0 };
	bool g_hasLinkRect = false;
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
			WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			95, 145, 120, 25,
			hWnd,
			reinterpret_cast<HMENU>(ID_CHANGE_SETTINGS),
			GetModuleHandleW(nullptr),
			nullptr);

		// Create the button used to reset settings (delete config and create new).
		CreateWindowW(
			L"BUTTON",
			L"Reset Settings",
			WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			95, 178, 120, 25,
			hWnd,
			reinterpret_cast<HMENU>(ID_CHANGE_SETTINGS_RESET),
			GetModuleHandleW(nullptr),
			nullptr);

		// Create the close button (no title bar, just a close button).
		CreateWindowW(
			L"BUTTON",
			L"X",
			WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			280, 10, 20, 20,
			hWnd,
			reinterpret_cast<HMENU>(ID_CHANGE_SETTINGS_CLOSE),
			GetModuleHandleW(nullptr),
			nullptr);
		return 0;

	case WM_DRAWITEM:
	{
		auto* drawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
		if (drawItem->CtlType != ODT_BUTTON)
			break;

		FillRect(drawItem->hDC, &drawItem->rcItem, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
		FrameRect(drawItem->hDC, &drawItem->rcItem, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));

		wchar_t buttonText[64] = {};
		GetWindowTextW(drawItem->hwndItem, buttonText, static_cast<int>(sizeof(buttonText) / sizeof(buttonText[0])));
		SetTextColor(drawItem->hDC, RGB(0, 255, 0));
		SetBkMode(drawItem->hDC, TRANSPARENT);
		DrawTextW(drawItem->hDC, buttonText, -1, &drawItem->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		if (drawItem->itemState & ODS_FOCUS)
		{
			RECT focusRect = drawItem->rcItem;
			InflateRect(&focusRect, -4, -4);
			DrawFocusRect(drawItem->hDC, &focusRect);
		}
		return TRUE;
	}

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

		// Configure green, transparent text so the black background remains visible.
		SetTextColor(hdc, RGB(0, 255, 0));
		SetBkMode(hdc, TRANSPARENT);

		const std::wstring versionLine = L"MiniMonitor Version: " + GetProgramVersion();

		// Build the help text.
		std::wstring info = versionLine;
		info += L"\n";
		info += kProjectUrl;
		info += L"\n\n"
			L"Controls:\n"
			L" - Drag window: Hold SHIFT and click & drag\n"
			L" - Double click: close window\n"
			L" - Right click: open this window (settings)\n"
			L"____________";

		// Draw the text with word wrapping inside the client area.
		RECT drawRect;
		GetClientRect(hWnd, &drawRect);
		drawRect.left += 10;
		drawRect.top += 10;
		drawRect.bottom -= 70;

		DrawTextW(
			hdc,
			info.c_str(),
			-1,
			&drawRect,
			DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_EXPANDTABS);

		TEXTMETRICW metrics{};
		GetTextMetricsW(hdc, &metrics);
		const int lineHeight = metrics.tmHeight + metrics.tmExternalLeading;

		SIZE urlSize{};
		GetTextExtentPoint32W(hdc, kProjectUrl, lstrlenW(kProjectUrl), &urlSize);
		g_linkRect.left = drawRect.left;
		g_linkRect.top = drawRect.top + lineHeight;
		g_linkRect.right = g_linkRect.left + urlSize.cx;
		g_linkRect.bottom = g_linkRect.top + lineHeight;
		g_hasLinkRect = true;

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

	case WM_NCLBUTTONDOWN:
		// Handle drag when clicking on the window frame (title bar area).
		g_isDragging = true;
		RECT windowRect;
		GetWindowRect(hWnd, &windowRect);
		g_dragOffset.x = LOWORD(lParam) - windowRect.left;
		g_dragOffset.y = HIWORD(lParam) - windowRect.top;
		return 0;

	case WM_LBUTTONDOWN:
	{
		POINT clickPoint = { LOWORD(lParam), HIWORD(lParam) };
		if (g_hasLinkRect && PtInRect(&g_linkRect, clickPoint))
		{
			ShellExecuteW(hWnd, L"open", kProjectUrl, nullptr, nullptr, SW_SHOWNORMAL);
			return 0;
		}

		// Handle drag when clicking anywhere in the client area.
		g_isDragging = true;
		RECT windowRect2;
		GetWindowRect(hWnd, &windowRect2);
		POINT pt = { LOWORD(lParam), HIWORD(lParam) };
		ClientToScreen(hWnd, &pt);
		g_dragOffset.x = pt.x - windowRect2.left;
		g_dragOffset.y = pt.y - windowRect2.top;
		return 0;
	}

	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT)
		{
			POINT cursorPos;
			GetCursorPos(&cursorPos);
			ScreenToClient(hWnd, &cursorPos);
			if (!g_isDragging && g_hasLinkRect && PtInRect(&g_linkRect, cursorPos))
			{
				SetCursor(LoadCursorW(nullptr, IDC_HAND));
			}
			else
			{
				SetCursor(LoadCursorW(nullptr, IDC_ARROW));
			}
			return TRUE;
		}
		break;

	case WM_MOUSEMOVE:
		if (g_isDragging)
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			ClientToScreen(hWnd, &pt);
			int newX = pt.x - g_dragOffset.x;
			int newY = pt.y - g_dragOffset.y;
			RECT rc;
			GetWindowRect(hWnd, &rc);
			int w = rc.right - rc.left;
			int h = rc.bottom - rc.top;
			MoveWindow(hWnd, newX, newY, w, h, TRUE);
		}
		return 0;

	case WM_LBUTTONUP:
		// Release drag tracking when mouse button is released.
		g_isDragging = false;
		return 0;

	case WM_DESTROY:
		// No additional cleanup is currently required for this window.
		return 0;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

