// readconfig.h
// Searches for the name of the setting, then returns the value.
// Input: "SettingName" Output: "SettingValue"
// Use like this:
// std::wstring wBgColor;
// ReadFromSettings(L"bg_color", wBgColor);
#pragma once
void EnsureConfigFileExists();
void ReadFromSettings(const std::wstring& SettingName, std::wstring& SettingValue);