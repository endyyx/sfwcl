#include <Windows.h>
#include <stdio.h>
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi")

int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR param, int) {
	TCHAR path[MAX_PATH];
	TCHAR params[MAX_PATH + 100];
	GetCurrentDirectory(MAX_PATH, path);
	PathAppendW(path, L"..\\Game\\");
	swprintf(params, L"\"%s\\\" /grant Users:(OI)(CI)F /T", path);
	MessageBoxW(0, params, 0, 0);
	ShellExecuteW(NULL, NULL, L"icacls.exe", params, NULL, SW_NORMAL);
	return 0;
}