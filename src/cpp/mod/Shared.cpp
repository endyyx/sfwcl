#include "Shared.h"
#include <Windows.h>
#include <stdio.h>
#include <map>
#include <WinInet.h>
#include <time.h>

template <int T> struct StaticBuffer{
	char content[T];
};

std::map<void*,StaticBuffer<25> > hData;

void* hookp(void *c,const void *d,const int sz){
	char *a=(char*)c;
	const char *b=(const char*)d;
	char *cave=new char[sz+6];
	memcpy(cave,a,sz);
	*(cave+sz)=(char)0xE9;
	*(long*)(cave+sz+1)=(a-cave)-5;
	DWORD fl=0;
	VirtualProtect(a,5,PAGE_READWRITE,&fl);
	*a=(char)0xE9;
	*(long*)(a+1)=b-a-5;
	VirtualProtect(a,5,fl,&fl);
	return (void*)(cave);
}
int getGameVer(const char *file){
    FILE *f=fopen(file,"rb"); 
    int c=0; 
    if(f){ 
		fseek(f,60,SEEK_SET); 
		if(!feof(f)) 
			c=fgetc(f); 
		fclose(f); 
		if(c==16) 
			return 6156; 
		else if(c==248) 
			return 5767; 
		else if(c=='\b')
			return 6729;
		else return 0;
	
    }
	return -1;
}
void hook(void* original_fn,void* new_fn){
	char *src=(char*)original_fn;
	char *dest=(char*)new_fn;
	DWORD fl=0;
	VirtualProtect(src,32,PAGE_READWRITE,&fl);
#ifdef IS64
	StaticBuffer<25> w;
	memcpy(w.content,src,12);
	hData[original_fn]=w;
	//x64 jump construction:
	src[0]=(char)0x48;	// 48 B8 + int64 = MOV RAX, int64
	src[1]=(char)0xB8;
	memcpy(src+2,&dest,8);
	src[10]=(char)0xFF;	// FF E0 = JMP RAX (absolute jump)
	src[11]=(char)0xE0;
#else
	unsigned long jmp_p=(unsigned long)((dest-src-5)&0xFFFFFFFF);
	StaticBuffer<25> w;
	memcpy(w.content,src,5);
	hData[original_fn]=w;
	//x86 jump construction:
	src[0]=(char)0xE9;	// E9 + int32 = JMP int32 (relative jump)
	memcpy(src+1,&jmp_p,4);
#endif
	VirtualProtect(src,32,fl,&fl);
}
void unhook(void *original_fn){
	DWORD fl=0;
	VirtualProtect(original_fn,32,PAGE_READWRITE,&fl);
	StaticBuffer<25> w=hData[original_fn];
#ifdef IS64
	memcpy(original_fn,w.content,12);
#else
	memcpy(original_fn,w.content,5);
#endif
	VirtualProtect(original_fn,32,fl,&fl);
}

std::string fastDownload(const char *url){
	HINTERNET hSession,hUrl;
	hSession = InternetOpen(NULL, 0, NULL, NULL, 0);
    hUrl = InternetOpenUrl(hSession, url, NULL, 0, 0, 0);
	DWORD readBytes = 0;
	std::string data="";
	char buffer[8192];
	do {
		::InternetReadFile(hUrl,buffer,8192,&readBytes);
		if(readBytes>0)
			data += std::string(buffer, buffer+readBytes);
	} while(readBytes!=0);

	InternetCloseHandle(hUrl);
    InternetCloseHandle(hSession);
	return data;
}

bool autoUpdateClient(){
	char cwd[5120];
	GetModuleFileNameA(0,cwd,5120);
	int last=-1;
	for(size_t i=0,j=strlen(cwd);i<j;i++){
		if(cwd[i]=='\\')
			last=(int)i;
	}
	if(last>=0)
		cwd[last]=0;
	char params[5120];
	sprintf(cwd,"%s\\..\\SfwClFiles\\",cwd);
	sprintf_s(params,"\"%s\" \"%s?%d\" \"%s\"","update","http://crymp.net/dl/client.zip",(int)time(0),cwd);
	//MessageBoxA(0, params, 0, 0);
	SHELLEXECUTEINFOA info;
	ZeroMemory(&info,sizeof(SHELLEXECUTEINFOA));
	info.lpDirectory=cwd;
	info.lpParameters=params;
	info.lpFile="MapDownloader.exe";
	info.nShow=SW_SHOW;
	info.cbSize=sizeof(SHELLEXECUTEINFOA);
	info.fMask=SEE_MASK_NOCLOSEPROCESS;
	info.hwnd=0;
	//MessageBoxA(0,cwd,0,0);
	if (!ShellExecuteExA(&info)) {
		char buffer[500];
		sprintf(buffer,"\nFailed to start auto updater, error code %d\n", GetLastError());
		MessageBoxA(0,buffer,0,MB_OK|MB_ICONERROR);
		return false;
	}
	return true;
}