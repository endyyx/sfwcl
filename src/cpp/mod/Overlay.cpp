#ifdef _USE_ADVANCED_FEATURES

#include "Overlay.h"

int width=800;
int height=600;
#define CENTERX (GetSystemMetrics(SM_CXSCREEN)/2)-(width/2)
#define CENTERY (GetSystemMetrics(SM_CYSCREEN)/2)-(height/2)
LPDIRECT3D9 d3d;
LPDIRECT3DDEVICE9 d3ddev;  
const MARGINS  margin = {0,0,width,height};
LPD3DXFONT pFont;

void InitD3D();
void Render();
void DrawString(int x, int y, DWORD color, LPD3DXFONT g_pFont, const char *fmt);

void InitD3D(HWND hWnd)
{
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = hWnd;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dpp.BackBufferWidth = width;
    d3dpp.BackBufferHeight =height;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3d->CreateDevice(D3DADAPTER_DEFAULT,
                      D3DDEVTYPE_HAL,
                      hWnd,
                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                      &d3dpp,
                      &d3ddev);
	D3DXCreateFont(d3ddev, 50, 0, FW_BOLD, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &pFont);
}

void DrawString(int x, int y, DWORD color, LPD3DXFONT g_pFont, const char *fmt)
{
    RECT FontPos = {x,y,x+120,y+16};
    char buf[1024] = {'\0'};
    va_list va_alist;
    va_start(va_alist, fmt);
    vsprintf_s(buf, fmt, va_alist);
    va_end(va_alist);
    g_pFont->DrawText(NULL, buf, -1, &FontPos, DT_NOCLIP, color);
}

void Render(){
    d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0,0, 0, 0), 1.0f, 0);
	d3ddev->BeginScene();
	//...
	DrawString(10,10,0xffffffff,pFont,"Hello world!");
    d3ddev->EndScene();
    d3ddev->Present(NULL, NULL, NULL, NULL);
}

LRESULT CALLBACK OverlayProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void OverlayWinMain(HWND hWnd){
	MSG msg;
	for(;;){
		SetWindowPos(hWnd ,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		//Render();
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if(msg.message == WM_QUIT)
            exit(0);
    }
}

HWND CreateOverlayWindow(HWND parent){
	static HWND exists=0;
	if(exists!=0) return exists;
	RECT rc;    
	if(parent){
		GetWindowRect(parent,&rc);
		width=rc.right - rc.left;
		height = rc.bottom - rc.top;
	} else {
		return 0;
	}
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = OverlayProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)RGB(0,0,0);
	wc.lpszClassName = "Overlay";
    RegisterClassEx(&wc);
	HWND hWnd = CreateWindowEx(0,"Overlay","",
                          WS_EX_TOPMOST|WS_POPUP,
                          rc.left, rc.top,
                          width, height,
                          NULL,
                          NULL,
                          GetModuleHandle(NULL),
                          NULL);
	SetWindowLong(hWnd, GWL_EXSTYLE,(int)GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED |WS_EX_TRANSPARENT);
	SetLayeredWindowAttributes(hWnd, RGB(0,0,0), 0, ULW_COLORKEY);
	SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA);
    ShowWindow(hWnd, SW_SHOW);
	InitD3D(hWnd);
	CreateThread(0,0,(LPTHREAD_START_ROUTINE)OverlayWinMain,hWnd,0,0);	
	exists=hWnd;
	return hWnd;
}
LRESULT CALLBACK OverlayProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
    case WM_PAINT:
        DwmExtendFrameIntoClientArea(hWnd, &margin);  
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc (hWnd, msg, wParam, lParam);
}  

#endif