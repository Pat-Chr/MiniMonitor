// readconfig.cpp
//reads the config file and creates the necessary variables for the program to use

#include <windows.h>
#include <cstdio>
#include <cstring>

// Ensure config.txt exists at startup; if not, create with default entries.
void EnsureConfigFileExists()
{
    const char* name = "config.txt";
    char path[MAX_PATH] = {0};

    // Pfad zum EXE-Ordner ermitteln
    char mod[MAX_PATH] = {0};
    if (GetModuleFileNameA(NULL, mod, MAX_PATH) != 0) {
        char* p = strrchr(mod, '\\');
        if (p) {
            *++p = '\0'; // behalte Ordnerpfad inklusive abschließendem '\'
            strcpy_s(path, sizeof(path), mod);
            strcat_s(path, sizeof(path), name);
        } else {
            strcpy_s(path, sizeof(path), name);
        }
    } else {
        // Fallback: aktuelles Verzeichnis
        GetCurrentDirectoryA(MAX_PATH, path);
        size_t len = strlen(path);
        if (len && path[len-1] != '\\') strcat_s(path, sizeof(path), "\\");
        strcat_s(path, sizeof(path), name);
    }

    // Existenz prüfen
    DWORD attrs = GetFileAttributesA(path);
    if (attrs != INVALID_FILE_ATTRIBUTES) return; // existiert bereits

    // Erstellen (OPEN_ALWAYS = öffne oder erstelle)
    HANDLE h = CreateFileA(path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        char buf[256];
        sprintf_s(buf, "EnsureConfigFileExists: CreateFileA fehlgeschlagen (err=%u) Pfad=%s\n", err, path);
        OutputDebugStringA(buf);
        return;
    }

    // Wenn Datei neu erstellt wurde (GetLastError != ERROR_ALREADY_EXISTS), schreibe Inhalt
    if (GetLastError() != ERROR_ALREADY_EXISTS) {
        const char* content = "text_color=FFFFFF\r\ntest1=text\r\ntest2=text2";
        DWORD written = 0;
        SetFilePointer(h, 0, NULL, FILE_BEGIN);
        WriteFile(h, content, (DWORD)strlen(content), &written, NULL);
    }

    CloseHandle(h);
}
