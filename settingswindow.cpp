// settingswindow.cpp
// Info window procedure for showing controls / settings

#include <windows.h>
#include <string>

#ifndef PROGRAM_VERSION
// Fallback, falls PROGRAM_VERSION nicht in einem anderen Header/Projekt definiert ist.
// Bei Bedarf hier anpassen (z.B. L"1.2.3" oder aus Ressourcen einlesen).
#define PROGRAM_VERSION L"0.0.0"
#endif

// Prototype for function implemented elsewhere in the project that fills an ANSI buffer
// with the configured text color (or related string). Adjust if the real signature differs.
void GetTextColorFromConfig(char* buffer, size_t size);

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
			L"MiniMonitor\n\n"
			L"Controls:\n\n"
			L" - Left click & drag: move window\n"
			L" - Double click: close window\n"
			L" - Right click: open this window (settings)\n\n"
			L"Current settings: (change them by editing the config.txt)\n";

		info += L"Text Color: ";
		info += wColor;
		info += std::wstring(L"\nVersion: ") + PROGRAM_VERSION;

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