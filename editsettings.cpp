// file: editsettings.cpp
// Edit settings window procedure for showing controls / settings

#include <windows.h>
#include <string>
#include <vector>
#include <winver.h>

#pragma comment(lib, "Version.lib")

//fetch the variables from the config file
void GetTextColorFromConfig(char* buffer, size_t size);
void GetBackgroundColorFromConfig(char* buffer, size_t size);

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

constexpr int ID_CHANGE_SETTINGS = 1001;
constexpr int ID_SAVE = 1002;
constexpr int ID_CANCEL = 1003;
constexpr int ID_TEXT_COLOR_EDIT = 1004;
constexpr int ID_BG_COLOR_EDIT = 1005;

HWND g_changeSettingsWindow = nullptr;

LRESULT CALLBACK ChangeSettingsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		// Create Text Color label and edit control
		CreateWindowW(
			L"STATIC",
			L"Text Color:",
			WS_CHILD | WS_VISIBLE | SS_LEFT,
			10, 20, 120, 20,
			hWnd,
			reinterpret_cast<HMENU>(0),
			GetModuleHandleW(nullptr),
			nullptr);

		CreateWindowW(
			L"EDIT",
			L"",
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
			150, 20, 100, 20,
			hWnd,
			reinterpret_cast<HMENU>(ID_TEXT_COLOR_EDIT),
			GetModuleHandleW(nullptr),
			nullptr);

		// Create Background Color label and edit control
		CreateWindowW(
			L"STATIC",
			L"Background Color:",
			WS_CHILD | WS_VISIBLE | SS_LEFT,
			10, 50, 120, 20,
			hWnd,
			reinterpret_cast<HMENU>(0),
			GetModuleHandleW(nullptr),
			nullptr);

		CreateWindowW(
			L"EDIT",
			L"",
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
			150, 50, 100, 20,
			hWnd,
			reinterpret_cast<HMENU>(ID_BG_COLOR_EDIT),
			GetModuleHandleW(nullptr),
			nullptr);

		// Create Save button
		CreateWindowW(
			L"BUTTON",
			L"Save",
			WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
			50, 100, 60, 25,
			hWnd,
			reinterpret_cast<HMENU>(ID_SAVE),
			GetModuleHandleW(nullptr),
			nullptr);

		// Create Cancel button
		CreateWindowW(
			L"BUTTON",
			L"Cancel",
			WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			150, 100, 60, 25,
			hWnd,
			reinterpret_cast<HMENU>(ID_CANCEL),
			GetModuleHandleW(nullptr),
			nullptr);

		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_SAVE:
		{
			// Read text color from edit control
			char textColor[256] = {};
			GetWindowTextA(GetDlgItem(hWnd, ID_TEXT_COLOR_EDIT), textColor, sizeof(textColor));

			// Read background color from edit control
			char bgColor[256] = {};
			GetWindowTextA(GetDlgItem(hWnd, ID_BG_COLOR_EDIT), bgColor, sizeof(bgColor));

			// Save to config file
			const char* name = "config.txt";
			char path[MAX_PATH] = { 0 };
			char mod[MAX_PATH] = { 0 };
			if (GetModuleFileNameA(NULL, mod, MAX_PATH) != 0)
			{
				char* p = strrchr(mod, '\\');
				if (p)
				{
					*++p = '\0';
					strcpy_s(path, sizeof(path), mod);
					strcat_s(path, sizeof(path), name);
				}
				else
				{
					strcpy_s(path, sizeof(path), name);
				}
			}
			else
			{
				GetCurrentDirectoryA(MAX_PATH, path);
				size_t len = strlen(path);
				if (len && path[len - 1] != '\\') strcat_s(path, sizeof(path), "\\");
				strcat_s(path, sizeof(path), name);
			}

			HANDLE hFile = CreateFileA(
				path,
				GENERIC_WRITE,
				0,
				NULL,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL);

			if (hFile != INVALID_HANDLE_VALUE)
			{
				SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

				// Write text_color line
				char writeBuffer[256];
				sprintf_s(writeBuffer, "text_color=%s\n", textColor);
				DWORD written = 0;
				WriteFile(hFile, writeBuffer, (DWORD)strlen(writeBuffer), &written, NULL);

				// Write bg_color line
				sprintf_s(writeBuffer, "bg_color=%s\n", bgColor);
				WriteFile(hFile, writeBuffer, (DWORD)strlen(writeBuffer), &written, NULL);

				CloseHandle(hFile);
			}

			DestroyWindow(hWnd);
			return 0;
		}
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
		300,
		200,
		owner,
		nullptr,
		GetModuleHandleW(nullptr),
		nullptr);

	if (g_changeSettingsWindow != nullptr)
	{
		ShowWindow(g_changeSettingsWindow, SW_SHOW);
		UpdateWindow(g_changeSettingsWindow);

		// Load current settings from config file into edit controls
		char textColorBuffer[256] = {};
		GetTextColorFromConfig(textColorBuffer, sizeof(textColorBuffer));
		SetWindowTextA(GetDlgItem(g_changeSettingsWindow, ID_TEXT_COLOR_EDIT), textColorBuffer);

		char bgColorBuffer[256] = {};
		GetBackgroundColorFromConfig(bgColorBuffer, sizeof(bgColorBuffer));
		SetWindowTextA(GetDlgItem(g_changeSettingsWindow, ID_BG_COLOR_EDIT), bgColorBuffer);
	}
}
