#include <Windows.h>
#include <stdio.h>
#include <Shlwapi.h>
#include <tchar.h>

#pragma comment(lib, "Shlwapi")

int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR param, int) {
	TCHAR path[MAX_PATH];
	TCHAR params[MAX_PATH + 100];
	GetCurrentDirectory(MAX_PATH, path);
	PathAppend(path, TEXT("..\\Game\\"));
	_stprintf(params, TEXT("\"%s\\\" /grant Users:(OI)(CI)F /T"), path);
	ShellExecute(NULL, NULL, TEXT("icacls.exe"), params, NULL, SW_NORMAL);
	return 0;
}