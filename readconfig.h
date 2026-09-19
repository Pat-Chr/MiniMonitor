// readconfig.h
#pragma once
void EnsureConfigFileExists();
void GetTextColorFromConfig(char* colorBuffer, size_t bufferSize);
void GetBackgroundColorFromConfig(char* colorBuffer, size_t bufferSize);