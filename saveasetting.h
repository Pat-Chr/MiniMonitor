#pragma once

#include <string>

/**
 * Saves a generic setting value to a configuration file.
 *
 * @param settingName Name of the setting (key) to save.
 * @param settingValue Value of the setting to store.
 *
 * @remarks
 * - Reads existing config.txt, updates or adds the specified setting line
 * - Supports any string value (including numeric values converted to strings)
 * - Writes updated content back to config.txt (truncating previous content)
 * - If setting doesn't exist, creates a new entry with the provided name and value
 * @returns void
 */

void SaveASetting(const std::wstring& settingName, const std::wstring& settingValue);