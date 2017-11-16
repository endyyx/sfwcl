#ifdef _USRDLL 
#define MAKE_DLL
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
// CMake is not used
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib,"miniunz")
#pragma comment(lib,"zlibstatic")
#endif

#include <Windows.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <time.h>
#include <fstream>
#include <string>
#include <direct.h>
#include <TlHelp32.h>
#include <commctrl.h>
#include <map>
#include "unzip.h"

#ifdef MAKE_DLL
#undef SendMessage
#define SendMessage /* do nothing */
#undef SetWindowText
#define SetWindowText(a,b) UpdateDLLProgress(a,b,false)
#ifdef MessageBoxA
#undef MessageBoxA
#endif
#define MessageBoxA(a,b,c,d) UpdateDLLProgress(a,b,true)
#endif

bool actAsUpdater = false;

#pragma comment(lib,"Ws2_32")
#pragma comment(lib,"urlmon")
using namespace std;

static char pbuffer[512];
char *msgbuffer = 0;

int gRETCODE=-1;

typedef void(__stdcall *PFNUPDATEPROGRESS)(const char*, bool);

int doUnzip(char *,char *);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void UpdateDLLProgress(HWND handle, const char *text, bool error);
int WorkerThread(void*);
HINSTANCE hInst;
HWND gHwnd = 0;
HWND hProgress = (HWND)1;
HWND hInfo = (HWND)1;
PFNUPDATEPROGRESS pfnUpdateProgress = 0;
const char *g_mapName = 0, *g_mapPath = 0, *g_cwd = 0;

class StatusCallback : public IBindStatusCallback {
public:
	HRESULT __stdcall QueryInterface(const IID &,void **) {
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef(void) {
        return 1;
    }
    ULONG STDMETHODCALLTYPE Release(void) {
        return 1;
    }
    HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD, IBinding *) {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE GetPriority(LONG *) {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE OnLowResource(DWORD ) {
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT , LPCWSTR ) {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD *, BINDINFO *) {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD , DWORD , FORMATETC *, STGMEDIUM *) {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE OnObjectAvailable(REFIID , IUnknown *) {
        return E_NOTIMPL;
    }
	virtual HRESULT WINAPI OnProgress(ULONG prog, ULONG progMax, ULONG status, LPCWSTR statusText){
		static int len=0;
		static double dnSpeed=0;
		static clock_t lastDnCheck=clock();
		static ULONG lastDn=0;
		double per=(100.0f*(double)prog)/(double)progMax;
		if(progMax>0 && prog!=progMax){
			if(clock()-lastDnCheck>=CLOCKS_PER_SEC){
				dnSpeed=(prog-lastDn)/1024.0f;
				lastDnCheck=clock();
				lastDn=prog;
				double toDown=progMax-prog;
				int estimated=99*60+99;
				if(dnSpeed>0) estimated=(int)((toDown/1024)/dnSpeed);
				if (prog < progMax) {
					for (int i = 0; i < len; i++)
						printf("\b \b");
				}
				float dividor=1;
				if(progMax>25000){
					dividor=progMax/25000.0f;
					prog=(int)((float)prog/dividor);
					progMax=25000;
				}
				char progstr[255];
				char units[]="KMGT";
				char unit=units[0];
				int unitAt=0;
				while(dnSpeed>=1024.0f && unitAt<4){
					unit=units[++unitAt];
					dnSpeed/=1024.0f;
				}

				SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, progMax));
				SendMessage(hProgress, PBM_SETPOS, prog, 0);
				sprintf(progstr,"Downloading map %3.2f %% (%6.2f %ciB/s, %02d:%02d)",per,dnSpeed,unit,estimated/60,estimated%60);
				SetWindowText(gHwnd,progstr);
				SetWindowText(hInfo,progstr);

				//len=printf("%.2f%% (%.1f kB/s, estimated time %02d:%02d)",per,dnSpeed,estimated/60,estimated%60);
			}
		} else if(prog==progMax && progMax>0){
			for(int i=0;i<len;i++)
				printf("\b \b\b");
			printf("100%%\n");
		}
		return S_OK;
	}
};

void UpdateDLLProgress(HWND handle, const char *text, bool error) {
	//if (handle == gHwnd) {
		if (pfnUpdateProgress && msgbuffer) {
			strcpy(msgbuffer, text);
			pfnUpdateProgress(msgbuffer, error);
		}
	//}
}

char *mklink(char *p,char *w,int *stg=0){
	char *v=w;
	int stage=0;
	while(*v){
		if(*v=='/' || *v=='\\')
			stage++;
		v++;
	}
	if(stage==0){
		if(stg) *stg=0;
		return 0;
	}
	int len=sprintf(pbuffer,actAsUpdater?"%s\\..\\Mods\\%s":"%s\\..\\Game\\%s",p,w);
	for(int i=0;i<len;i++)
		if(pbuffer[i]=='/')
			pbuffer[i]='\\';
	if(stg) *stg=stage;
	return pbuffer;
}

bool IsProcessActive(const char *proc){
	HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) {
        CloseHandle(hProcessSnap);
		return false;
    } else {
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hProcessSnap, &pe32)) {
            if (!_stricmp(pe32.szExeFile,proc)) {
                CloseHandle(hProcessSnap);
				return true;
            } else {
                while (Process32Next(hProcessSnap, &pe32)) { 
                    if (!_stricmp(pe32.szExeFile,proc)) {
						CloseHandle(hProcessSnap);
						return true;
					}
                }
            }
        }
		CloseHandle(hProcessSnap);
    }
	return false;
}

#ifndef MAKE_DLL
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrv,LPSTR cmdLine,int nCmdShow){
	actAsUpdater = __argc>=2 && !strcmp(__argv[1],"update");
	hInst=hInstance;
	WNDCLASSEXA wc;
	HWND hWnd;
	MSG msg;
	wc.cbSize=sizeof(wc);
	wc.style=0;
	wc.lpfnWndProc=WndProc;
	wc.hInstance=hInstance;
	wc.lpszClassName=actAsUpdater?"Auto-update":"Map downloader";
	wc.hCursor=LoadCursor(hInstance,IDC_ARROW);
	wc.hIcon=LoadIcon(hInstance,IDI_APPLICATION);
	wc.hIconSm=LoadIcon(hInstance,IDI_APPLICATION);
	wc.lpszMenuName=0;
	wc.cbWndExtra=0;
	wc.cbClsExtra=0;
	wc.hbrBackground=(HBRUSH)(COLOR_WINDOW);
	if(!RegisterClassExA(&wc)){
        MessageBoxA(NULL,"Failed to register window!","Error!",MB_ICONEXCLAMATION | MB_OK);
        return 17;
    }
	hWnd = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        actAsUpdater?"Auto-update":"Map downloader",
        actAsUpdater?"Auto-update":"Map downloader",
        WS_CAPTION|WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 360, 150,
        0, 0, hInstance, 0);
	if(!hWnd){
		MessageBoxA(0,"Failed to create window!","Error!",MB_ICONEXCLAMATION | MB_OK);
		return 18;
	}
	ShowWindow(hWnd,nCmdShow);
	UpdateWindow(hWnd);
	gHwnd=hWnd;
	while(GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	return gRETCODE;
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
	HWND spam=0;
    switch(msg){
		case WM_CREATE:
			hProgress=CreateWindowEx(0,PROGRESS_CLASS,0,
									WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
									10, 10, 325, 40,
									hwnd, NULL, hInst, NULL);
			hInfo=CreateWindowA("static", "ST_INFO",
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                              15, 55, 350, 24,
                              hwnd, (HMENU)(123),
                              (HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
			spam=CreateWindowA("static", "ST_SPAM",
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                              30, 80, 350, 24,
                              hwnd, (HMENU)(123),
                              (HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
			SetWindowText(hInfo,"Action: none");
			SetWindowText(spam,"Game will start as soon as possible");
			CreateThread(0,0,(LPTHREAD_START_ROUTINE)WorkerThread,0,0,0);
			break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
			break;
        case WM_DESTROY:
            PostQuitMessage(0);
			break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
			break;
    }
    return 0;
}
#else
extern "C" {
	__declspec(dllexport) void __cdecl SetUpdateProgressCallback(void *ptr) {
		pfnUpdateProgress = (PFNUPDATEPROGRESS)ptr;
	}
	__declspec(dllexport) int __cdecl DownloadMap(const char *mapName, const char *mapPath, const char *cwd) {
		if (!msgbuffer) {
			msgbuffer = new char[65536];
		}
		g_mapName = mapName;
		g_mapPath = mapPath;
		g_cwd = cwd;
		//CreateThread(0, 0, (LPTHREAD_START_ROUTINE)WorkerThread, 0, 0, 0);
		return WorkerThread(0);
	}
}
#endif
int doUnzip(char *fpath, char *path) {

	char originalDirectory[MAX_PATH];

	SendMessage(hProgress, PBM_SETRANGE, 20, MAKELPARAM(0, 100));
	SendMessage(hProgress, PBM_SETSTEP, 1, 0);
	SendMessage(hProgress, PBM_STEPIT, 0, 0);

	int retCode = 0;
	unzFile f = unzOpen(fpath);
	unz_global_info global_info;
	if (unzGetGlobalInfo(f, &global_info) != UNZ_OK) {
		unzClose(f);
		printf("\nFailed to get ZIP file info, error: %s\n", strerror(errno));
		//while (getchar() != '\n') {}
		retCode = 4; goto _end_;
	}
	char read_buffer[16384];
	uLong i;
	SetWindowText(gHwnd, "Decompressing archive...");
	FILE *out = 0;
	GetCurrentDirectory(MAX_PATH, originalDirectory);
	for (i = 0; i < global_info.number_entry; ++i)
	{
		unz_file_info file_info;
		char filename[255];
		if (unzGetCurrentFileInfo(f, &file_info, filename, 255, NULL, 0, NULL, 0) != UNZ_OK) {
			unzClose(f);
			printf("\nFailed to get ZIP file info, error: %s\n", strerror(errno));
			//while (getchar() != '\n') {}
			retCode = 4; goto _end_;
		}
		char tmpfname[255];
		const size_t filename_length = strlen(filename);
		if (filename_length>16) {
			sprintf_s(tmpfname, "...%s", filename + (filename_length - 16));
		}
		else strncpy(tmpfname, filename, 255);
		char cdcmd[255];
		sprintf(cdcmd, actAsUpdater ? "%s\\..\\Mods\\" : "%s\\..\\Game\\", path);
		//system(cdcmd);
		SetCurrentDirectory(cdcmd);
		if (filename[filename_length - 1] == '\\' || filename[filename_length - 1] == '/') {
			printf("Extracting directory: %s\n", mklink(path, filename));
			_mkdir(mklink(path, filename));
		}
		else {
			int stage = 0;
			char *exn = mklink(path, filename, &stage);
			if (!exn) {
				printf("Removing potentionally dangerous file: %s\n", filename);
			}
			else {
				printf("Extracting file (st: %d): %s\n", stage, exn);
				if (unzOpenCurrentFile(f) != UNZ_OK) {
					unzClose(f);
					printf("\nFailed to open the file, error: %s\n", strerror(errno));
					//while (getchar() != '\n') {}
					retCode = 5; goto _end_;
				}
				out = fopen((mklink(path, filename)), "wb");
				if (!out) {
					unzCloseCurrentFile(f);
					unzClose(f);
					printf("\nFailed to output the file, error: %s\n", strerror(errno));
					//while (getchar() != '\n') {}
					retCode = 6; goto _end_;
				}
				int error = UNZ_OK;
				float dividor = 1.0f;
				int stepsz = 16384;
				int fsz = file_info.uncompressed_size;
				if (fsz>25000) {
					dividor = fsz / 25000.0f;
					stepsz = (int)((float)stepsz / dividor);
				}

				SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, fsz));
				SendMessage(hProgress, PBM_SETSTEP, stepsz, 0);

				char infoMsg[255];
				float state = 0;
				sprintf(infoMsg, "Extracting %s - 0.00%%", tmpfname);
				SetWindowText(hInfo, infoMsg);
				do {
					error = unzReadCurrentFile(f, read_buffer, 16384);
					if (error<0) {
						unzCloseCurrentFile(f);
						unzClose(f);
						printf("\nFailed to read the file, error: %s\n", strerror(errno));
						//while (getchar() != '\n') {}
						retCode = 7; goto _end_;
					}
					if (error>0)
						fwrite(read_buffer, error, 1, out);
					SendMessage(hProgress, PBM_STEPIT, 0, 0);
					state += 16384;
					if (state>file_info.uncompressed_size)
						state = (float)file_info.uncompressed_size;
					float per = 100 * state / file_info.uncompressed_size;
					sprintf(infoMsg, "Extracting %s - %.2f%%", tmpfname, per);
					SetWindowText(hInfo, infoMsg);
				} while (error>0);
				fclose(out);
				out = 0;
			}
		}
		unzCloseCurrentFile(f);
		if ((i + 1)<global_info.number_entry) {
			if (unzGoToNextFile(f) != UNZ_OK) {
				unzClose(f);
				printf("\nFailed to find next file, error: %s\n", strerror(errno));
				//while (getchar() != '\n') {}
				retCode = 8; goto _end_;
			}
		}
	}
	SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	SendMessage(hProgress, PBM_SETPOS, 100, 0);
_end_:
	if (out) {
		fclose(out);
	}
	unzClose(f);
	SetCurrentDirectory(originalDirectory);
	return retCode;
}
int WorkerThread(void*){
	char *ver = 0;
	char mapn[512];
	char path[512];
	char mapdl[512];
	int retCode = 0;
	int i = 0;
	int len = 0;
#ifndef MAKE_DLL
	if(actAsUpdater){
		while(IsProcessActive("Crysis.exe")){
			SetWindowText(hInfo,"Waiting for game to close...");
			Sleep(1000);
		}
		SetWindowText(hInfo,"Updating a client...");
	}
	int argc=__argc;
	char **argv=__argv;
	if(argc<3){
		PostQuitMessage(19);
		return 19;
	}
	len=(int)strlen(argv[1]);
	strncpy_s(mapn,argv[1],512);
	strncpy_s(mapdl,argv[2],512);

	for(int i=0,j=strlen(mapn);i<j;i++){
		if(mapn[i]=='|'){
			mapn[i]=0;
			ver=mapn+i+1;
		}
	}
	for(int i=3;i<argc;i++){
		if(i==3)
			strncpy_s(path,argv[i],512);
		else{
			strncat_s(path," ",1);
			strncat_s(path,argv[i],512);
		}
	}
#else
	strncpy_s(mapn, g_mapName, 512);
	strncpy_s(mapdl, g_mapPath, 512);
	strncpy_s(path, g_cwd, 512);
	for (size_t i = 0, j = strlen(mapn); i<j; i++) {
		if (mapn[i] == '|') {
			mapn[i] = 0;
			ver = mapn + i + 1;
		}
	}
#endif
	char mpath[512];
	srand((unsigned int)time(0));
	char fname[255];
	printf("Map: %s\nURL: %s\nPath: %s\n",mapn,mapdl,path);
	ZeroMemory(fname,sizeof(fname));
	strcpy(fname,"_mapdl.tmp");
	//sprintf_s(fname,"%s.pak",fname);
	printf("TmpFile: %s\n",fname);
	for(size_t i=0,j=strlen(mapn);i<j;i++)
		mapn[i]=mapn[i]=='/'?'\\':mapn[i];
	sprintf_s(mpath,"%s\\..\\Game\\Levels\\%s",path,mapn);
	char fpath[512];
	sprintf_s(fpath,"%s\\..\\Game\\%s",path,fname);

	FILE *f=fopen(fpath,"rb");
	if(f){
		fclose(f);
		_unlink(fpath);
		printf("Removed old TMP file\n");
	}

	StatusCallback stat;
	IBindStatusCallback *callback=(IBindStatusCallback*)&stat;
	printf("Downloading file: ");
//retry:
	HRESULT ok=URLDownloadToFileA(0,mapdl,fpath,0,&stat);
	if(SUCCEEDED(ok)){
		printf("Map was successfuly downloaded! \nExtracting...\n");
		retCode=doUnzip(fpath,path);
	} else if (ok==E_OUTOFMEMORY){
		MessageBoxA(gHwnd,"Invalid buffer length, or there not enough memory to complete the operation.","Error!",MB_ICONERROR|MB_OK);
		retCode = ok; goto _end_;
	} else if (ok==INET_E_AUTHENTICATION_REQUIRED){
		MessageBoxA(gHwnd,"Authentication is required to download this file.","Error!",MB_ICONERROR|MB_OK);
		retCode = ok; goto _end_;
	} else if (ok==INET_E_CANNOT_CONNECT){
		MessageBoxA(gHwnd,"Failed to connect to the internet.","Error!",MB_ICONERROR|MB_OK);
		retCode = ok; goto _end_;
	} else if (ok==INET_E_CONNECTION_TIMEOUT){
		MessageBoxA(gHwnd,"Internet connection has timed out.","Error!",MB_ICONERROR|MB_OK);
		retCode = ok; goto _end_;
	} else if (ok==INET_E_DATA_NOT_AVAILABLE){
		MessageBoxA(gHwnd,"The requested data are not available.","Error!",MB_ICONERROR|MB_OK);
		retCode = ok; goto _end_;
	} else if (ok==INET_E_DOWNLOAD_FAILURE){
		MessageBoxA(gHwnd,"The download has failed (the connection was interrupted).","Error!",MB_ICONERROR|MB_OK);
		retCode = ok; goto _end_;
	} else if (ok==INET_E_INVALID_REQUEST){
		MessageBoxA(gHwnd,"The download request is invalid.","Error!",MB_ICONERROR|MB_OK);
		retCode = ok; goto _end_;
	} else if (ok==INET_E_INVALID_URL){
		MessageBoxA(gHwnd,"The URL has invalid format.","Error!",MB_ICONERROR|MB_OK);
		retCode = ok; goto _end_;
	} else if (ok==INET_E_OBJECT_NOT_FOUND){
		MessageBoxA(gHwnd,"The requested file was not found (invalid download link).","Error!",MB_ICONERROR|MB_OK);
		retCode = ok; goto _end_;
	} else if (ok==INET_E_RESOURCE_NOT_FOUND){
		MessageBoxA(gHwnd,"The server or proxy was not found.","Error!",MB_ICONERROR|MB_OK);
		retCode = ok; goto _end_;
	} else {
		char errmsg[255];
		sprintf(errmsg,"Failed to download the file, return code %#X\n",ok);
		MessageBoxA(gHwnd,errmsg,"Error!",MB_ICONERROR|MB_OK);
		retCode = ok; goto _end_;
	}
_end_:
	_unlink(fpath);
	gRETCODE=retCode;
	if(!retCode){
		if(actAsUpdater){
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
			sprintf(cwd,"%s\\..\\Bin32\\",cwd);
			sprintf_s(params,"-mod sfwcl");
			SHELLEXECUTEINFOA info;
			ZeroMemory(&info,sizeof(SHELLEXECUTEINFOA));
			info.lpDirectory=cwd;
			info.lpParameters=params;
			info.lpFile="Crysis.exe";
			info.nShow=SW_SHOW;
			info.cbSize=sizeof(SHELLEXECUTEINFOA);
			info.fMask=SEE_MASK_NOCLOSEPROCESS;
			info.hwnd=0;
			//MessageBoxA(0,cwd,0,0);
			if (!ShellExecuteExA(&info)) {
				char buffer[500];
				sprintf(buffer,"\nFailed to start game, error code %d\n", GetLastError());
				MessageBoxA(0,buffer,0,MB_OK|MB_ICONERROR);
			}
		} else {
			map<string,string> maps;
			char lpath[512];
			sprintf_s(lpath,"%s\\..\\Game\\_levels.dat",path);
			FILE *f=fopen(lpath,"r");
			if(f){
				char name[255];
				char veri[255];
				fseek(f,SEEK_SET,0);
				while(!feof(f)){
					fscanf(f,"%s %s",name,veri);
					maps[name]=veri;
				}
				fclose(f);
			}
			if(ver)
				maps[mapn]=ver;
			f=fopen(lpath,"w");
			if(f){
				for(map<string,string>::iterator it=maps.begin();it!=maps.end();it++){
					fprintf(f,"%s %s\n",it->first.c_str(),it->second.c_str());
				}
				fclose(f);
			}
		}
	}
#ifndef MAKE_DLL
	PostQuitMessage(retCode);
	SendMessage(gHwnd,WM_DESTROY,0,0);
#endif
	return retCode;
}
