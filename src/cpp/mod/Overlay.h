#ifdef _USE_ADVANCED_FEATURES

#include <windows.h>
#include <d3d9.h>
#include <iostream>
#include <Dwmapi.h> 
#include <d3dx9.h>
#include <TlHelp32.h> 

#pragma comment(lib,"D3d9")
#pragma comment(lib,"Dwmapi")
#pragma comment(lib,"d3dx9")

void InitD3D();
void Render();
HWND CreateOverlayWindow(HWND parent);

#endif