// readconfig.h
// Searches for the name of the setting, then returns the value.
// Input: "SettingName" Output: "SettingValue"
// Use like this:
// std::wstring wBgColor;
// ReadFromSettings(L"bg_color", wBgColor);
#pragma once
void EnsureConfigFileExists();
void GetTextColorFromConfig(char* colorBuffer, size_t bufferSize); // Will also be removed using the same method as background bolor.
// void GetBackgroundColorFromConfig(char* colorBuffer, size_t bufferSize);  // We are not using this anymore.
void ReadFromSettings(const std::wstring& SettingName, std::wstring& SettingValue);