#ifdef _USE_ADVANCED_FEATURES

#include "DXOverlay.h"
#include <Windows.h>
#include <d3d9.h>
#include <stdio.h>

bool Compare(const BYTE* pData, const BYTE* bMask, const char* szMask)
{
	VirtualProtect((LPVOID)pData,strlen(szMask)*2,PAGE_READWRITE,0);
    for(;*szMask;++szMask,++pData,++bMask)
        if(*szMask=='x' && *pData!=*bMask ) 
            return false;
    return (*szMask) == NULL;
}
DWORD FindPattern(DWORD dwAddress,DWORD dwLen,BYTE *bMask,char* szMask)
{
	char msg[300];
	sprintf(msg,"FindPattern %p,%d,%s called",dwAddress,dwLen,szMask);
	MessageBoxA(0,msg,0,0);
    for(DWORD i=0;i<dwLen;i++)
        if(Compare((BYTE*)(dwAddress+i),bMask,szMask))
            return (DWORD)(dwAddress+i);
    return 0;
}

HRESULT (APIENTRY *OnEndSceneCallback)(LPDIRECT3DDEVICE9 pDevice);
typedef HRESULT (APIENTRY *PFNOES)(LPDIRECT3DDEVICE9 pDevice);

HRESULT APIENTRY OnEndSceneFunc(LPDIRECT3DDEVICE9 pDevice){
	MessageBoxA(0,"Ej",0,0);
	return OnEndSceneCallback(pDevice);
}

void DoOverlay(){
	MessageBoxA(0,"Do overlay called!",0,0);
	DWORD* vtbl = 0;     
    DWORD table = FindPattern((DWORD)GetModuleHandle("d3d9.dll"), 0x128000,     (PBYTE)"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86", "xx????xx????xx");
    char msg[500];
	sprintf(msg,"table: %p",table);
	MessageBoxA(0,msg,0,0);
	memcpy(&vtbl, (void*)(table+2), 4);
    DWORD OESAddr = vtbl[42];    
	sprintf(msg,"addr: %p",OESAddr);
	MessageBoxA(0,msg,0,0);
    if (OESAddr)
    {
        OnEndSceneCallback = (PFNOES)(hookp((PBYTE)OESAddr,(PBYTE)OnEndSceneFunc,5));
    }
}

#endif