// settingswindow.cpp
// Info window procedure for showing controls / settings

#include <windows.h>
#include <string>
#include <vector>
#include <winver.h>

void GetTextColorFromConfig(char* buffer, size_t size);

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

LRESULT CALLBACK InfoWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
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

		char colorBuffer[128] = {0};
		GetTextColorFromConfig(colorBuffer, sizeof(colorBuffer));

		// Convert config text (ANSI) to a wide string for DrawTextW
		WCHAR wColor[128] = {0};
		MultiByteToWideChar(CP_ACP, 0, colorBuffer, -1, wColor, _countof(wColor));

		std::wstring info =
			L"Controls:\n"
			L" - Left click & drag: move window\n"
			L" - Double click: close window\n"
			L" - Right click: open this window (settings)\n"
			L"____________\n\nCurrent settings:\n(change them by editing the config.txt)";

		info += L"\nText Color: ";
		info += wColor;
		info += L"\n____________";
		info += L"\n\n\nVersion: ";
		info += GetProgramVersion();

		DrawTextW(hdc, info.c_str(), -1, &rect, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_EXPANDTABS);

		EndPaint(hWnd, &ps);
	}
	return 0;

	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;

	case WM_DESTROY:
		// Child/settings window: do not call PostQuitMessage
		return 0;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}