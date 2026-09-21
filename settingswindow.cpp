// settingswindow.cpp
// Info window procedure for showing controls / settings

#include <windows.h>
#include <string>
#include <vector>
#include <winver.h>

//fetch the variables from the config file
void GetTextColorFromConfig(char* buffer, size_t size);
void GetBackgroundColorFromConfig(char* buffer, size_t size);

#pragma comment(lib, "Version.lib")

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

namespace
{
	constexpr int ID_CHANGE_SETTINGS = 1001;
	constexpr int ID_SAVE = 1002;
	constexpr int ID_CANCEL = 1003;

	HWND g_changeSettingsWindow = nullptr;

	LRESULT CALLBACK ChangeSettingsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CREATE:
			CreateWindowW(
				L"BUTTON",
				L"Save",
				WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
				80, 60, 80, 25,
				hWnd,
				reinterpret_cast<HMENU>(ID_SAVE),
				GetModuleHandleW(nullptr),
				nullptr);

			CreateWindowW(
				L"BUTTON",
				L"Cancel",
				WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
				170, 60, 80, 25,
				hWnd,
				reinterpret_cast<HMENU>(ID_CANCEL),
				GetModuleHandleW(nullptr),
				nullptr);

			return 0;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case ID_SAVE:
				// Settings saving will be implemented later.
				DestroyWindow(hWnd);
				return 0;

			case ID_CANCEL:
				DestroyWindow(hWnd);
				return 0;
			}
			break;

		case WM_CLOSE:
			DestroyWindow(hWnd);
			return 0;

		case WM_DESTROY:
			g_changeSettingsWindow = nullptr;
			return 0;
		}

		return DefWindowProcW(hWnd, message, wParam, lParam);
	}

	void OpenChangeSettingsWindow(HWND owner)
	{
		if (g_changeSettingsWindow != nullptr)
		{
			SetForegroundWindow(g_changeSettingsWindow);
			return;
		}

		static bool classRegistered = false;

		if (!classRegistered)
		{
			WNDCLASSW windowClass = {};
			windowClass.hInstance = GetModuleHandleW(nullptr);
			windowClass.lpfnWndProc = ChangeSettingsWndProc;
			windowClass.lpszClassName = L"MiniMonitorChangeSettingsWindow";
			windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
			windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

			RegisterClassW(&windowClass);
			classRegistered = true;
		}

		POINT cursorPosition;
		GetCursorPos(&cursorPosition);

		g_changeSettingsWindow = CreateWindowExW(
			0,
			L"MiniMonitorChangeSettingsWindow",
			L"Change settings",
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
			cursorPosition.x,
			cursorPosition.y,
			400,
			300,
			owner,
			nullptr,
			GetModuleHandleW(nullptr),
			nullptr);

		if (g_changeSettingsWindow != nullptr)
		{
			ShowWindow(g_changeSettingsWindow, SW_SHOW);
			UpdateWindow(g_changeSettingsWindow);
		}
	}
}

LRESULT CALLBACK InfoWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		CreateWindowW(
			L"BUTTON",
			L"Change settings",
			WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			10, 10, 130, 25,
			hWnd,
			reinterpret_cast<HMENU>(ID_CHANGE_SETTINGS),
			GetModuleHandleW(nullptr),
			nullptr);
		return 0;

	case WM_SIZE:
	{
		HWND button = GetDlgItem(hWnd, ID_CHANGE_SETTINGS);

		if (button != nullptr)
		{
			RECT rect;
			GetClientRect(hWnd, &rect);

			const int buttonWidth = 130;
			const int buttonHeight = 25;
			const int margin = 10;

			MoveWindow(
				button,
				rect.right - buttonWidth - margin,
				rect.bottom - buttonHeight - margin,
				buttonWidth,
				buttonHeight,
				TRUE);
		}

		return 0;
	}

	case WM_COMMAND:
		if (LOWORD(wParam) == ID_CHANGE_SETTINGS &&
			HIWORD(wParam) == BN_CLICKED)
		{
			OpenChangeSettingsWindow(hWnd);
			return 0;
		}
		break;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		RECT rect;
		GetClientRect(hWnd, &rect);

		HBRUSH hBg = CreateSolidBrush(RGB(0, 0, 0));
		FillRect(hdc, &rect, hBg);
		DeleteObject(hBg);

		SetTextColor(hdc, RGB(255, 255, 255));
		SetBkMode(hdc, TRANSPARENT);

		char colorBuffer[128] = { 0 };
		GetTextColorFromConfig(colorBuffer, sizeof(colorBuffer));

		WCHAR wColor[128] = { 0 };
		MultiByteToWideChar(
			CP_ACP,
			0,
			colorBuffer,
			-1,
			wColor,
			_countof(wColor));

		char bg_Color[128] = { 0 };
		GetBackgroundColorFromConfig(bg_Color, sizeof(bg_Color));

		WCHAR wBgColor[128] = { 0 };
		MultiByteToWideChar(
			CP_ACP,
			0,
			bg_Color,
			-1,
			wBgColor,
			_countof(wBgColor));

		std::wstring info =
			L"Controls:\n"
			L" - Drag window: Hold SHIFT and click & drag\n"
			L" - Double click: close window\n"
			L" - Right click: open this window (settings)\n"
			L"____________\n\nCurrent settings:\n"
			L"(change them by editing the config.txt)";

		info += L"\nText Color: ";
		info += wColor;
		info += L"\nBackground Color: ";
		info += wBgColor;
		info += L"\n____________";
		info += L"\n\n\nMiniMonitor Version: ";
		info += GetProgramVersion();

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
		DestroyWindow(hWnd);
		return 0;

	case WM_DESTROY:
		return 0;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}