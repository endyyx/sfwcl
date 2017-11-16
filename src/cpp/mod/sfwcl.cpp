#include "Shared.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sstream>
#include "Mutex.h"
//#include <mutex>

#define CLIENT_BUILD 1001
//#define AUTO_UPDATE

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#include <CryModuleDefs.h>
#include <platform_impl.h>
#include <IGameFramework.h>
#include <ISystem.h>
#include <IScriptSystem.h>
#include <IConsole.h>
#include <I3DEngine.h>
#include <IFont.h>
#include <IUIDraw.h>
#include <IFlashPlayer.h>
#include <WinSock2.h>
#include <ShlObj.h>
#include "CPPAPI.h"
#include "Socket.h"
#include "Structs.h"
#include "Atomic.h"
CPPAPI *luaApi=0;
Socket *socketApi=0;
ISystem *pSystem=0;
IConsole *pConsole=0;
GAME_32_6156 *pGameGlobal=0;
IGame *pGame = 0;
IScriptSystem *pScriptSystem=0;
IGameFramework *pGameFramework=0;
IFlashPlayer *pFlashPlayer=0;
AsyncData *asyncQueue[MAX_ASYNC_QUEUE+1];
Atomic<const char*> mapDlMessage(0);
std::map<std::string, std::string> asyncRetVal;
int asyncQueueIdx = 0;

typedef void (__fastcall *PFNLOGIN)(void*,void*,const char*);		//HUD::OnLogin
typedef void (__fastcall *PFNSHLS)(void*,void*);					//HUD::ShowLoginScreen
typedef void (__fastcall *PFNJS)(void*,void*);						//HUD::JoinServer
typedef bool (__fastcall *PFNGSS)(void*,void*,SServerInfo&);		//HUD::GetSelectedServer
typedef void (__fastcall *PFNDE)(void*,void*,EDisconnectionCause, bool,const char*);	//HUD::OnDisconnectError
typedef void (__fastcall *PFNSE)(void*,void*,const char*,int);		//HUD::ShowError
typedef void* (__fastcall *PFNGM)(void*, void*);					//CGame::GetMenu
typedef void* (__fastcall *PFNGMS)(void*, void*, EMENUSCREEN);		//FlashObj::GetMenuScreen
typedef bool (__fastcall *PFNMIL)(void*, void*);					//FlashScreen::IsLoaded
typedef int (__fastcall *PFNGU)(void*, void*, bool, unsigned int);	//CGame::Update

int GAME_VER=6156;

Mutex g_mutex;
bool g_gameFilesWritable;
unsigned int g_objectsInQueue=0;

PFNLOGIN pLoginCave=0;
PFNLOGIN pLoginSuccess=0;
PFNSHLS	pShowLoginScreen=0;
PFNJS pJoinServer=0;
PFNGSS pGetSelectedServer=0;
PFNDE pDisconnectError=0;
PFNSE pShowError=0;
PFNGM pGetMenu = 0;
PFNGMS pGetMenuScreen = 0;
PFNMIL pMenuIsLoaded = 0;
PFNGU pGameUpdate = 0;
PFNSETUPDATEPROGRESSCALLBACK pfnSetUpdateProgressCallback = 0;
PFNDOWNLOADMAP pfnDownloadMap = 0;

HMODULE hMapDlLib = 0;

void *m_ui;
char SvMaster[255]="m.crymp.net";

bool TestGameFilesWritable();

void OnUpdate(float frameTime);

void __stdcall MapDownloadUpdateProgress(const char *msg, bool error) {
	mapDlMessage.set(msg);
}

void CommandClMaster(IConsoleCmdArgs *pArgs){
	if (pArgs->GetArgCount()>1)
	{
		const char *to=pArgs->GetCommandLine()+strlen(pArgs->GetArg(0))+1;
		strncpy(SvMaster, to, sizeof(SvMaster));
	}
	pScriptSystem->BeginCall("printf");
	char buff[50];
	sprintf(buff,"$0    cl_master = $6%s",SvMaster);
	pScriptSystem->PushFuncParam("%s");
	pScriptSystem->PushFuncParam(buff);
	pScriptSystem->EndCall();
}
void CommandRldMaps(IConsoleCmdArgs *pArgs){
	ILevelSystem *pLevelSystem = pGameFramework->GetILevelSystem();
	if(pLevelSystem){
		pLevelSystem->Rescan();
	}
}

void __fastcall OnShowLoginScr(void *self, void *addr) {
	pScriptSystem->BeginCall("OnShowLoginScreen");
	pScriptSystem->PushFuncParam(true);
	pScriptSystem->EndCall();
	pFlashPlayer = *(IFlashPlayer**)(self);
	pFlashPlayer->Invoke1("_root.Root.MainMenu.MultiPlayer.MultiPlayer.gotoAndPlay", "internetgame");
}

IFlashPlayer *GetFlashPlayer(int offset = 0, int pos = -1) {
	if (!pGetMenu) return pFlashPlayer;
	void *pMenu = pGetMenu(pGame, pGetMenu);
	for (int i = offset; i < 6; i++) {
		if (pos != -1 && i != pos) continue;
#ifdef IS64
		MENU_SCREEN *pMenuScreen = 0;
		pMenuScreen = getField(MENU_SCREEN*, pMenu, 0x80);
		//FLASH_OBJ_64_6156 *pFlashObj = (FLASH_OBJ_64_6156*)pMenu;
		//pMenuScreen = pFlashObj->arr[i];
#else
		MENU_SCREEN *pMenuScreen = (MENU_SCREEN*)pGetMenuScreen(pMenu, pGetMenuScreen, (EMENUSCREEN)i);
#endif
		if (pMenuScreen && pMenuIsLoaded(pMenuScreen, pMenuIsLoaded)) {
			return getField(IFlashPlayer*, pMenuScreen, sizeof(void*));
		}
	}
	return 0;
}

void ToggleLoading(const char *text, bool loading, bool reset) {
	static bool isActive = false;
	if (loading && isActive) {
		reset = false;
	}
	isActive = loading;
	pFlashPlayer = GetFlashPlayer();
	if (pFlashPlayer) {
		if (reset)
			pFlashPlayer->Invoke1("showLOADING", loading);
		if (loading) {
			SFlashVarValue args[] = { text,false };
			pFlashPlayer->Invoke("setLOADINGText", args, sizeof(args) / sizeof(args[0]));
		}
	}
}
bool checkFollowing = false;
bool __fastcall GetSelectedServer(void *self, void *addr, SServerInfo& server) {
	m_ui = self;
	unhook(pGetSelectedServer);
	bool result = pGetSelectedServer(self, addr, server);
	hook(pGetSelectedServer, GetSelectedServer);
	if (checkFollowing) {
		char sz_ip[30];
		int ip = server.m_publicIP;
		int port = server.m_publicPort;
#ifdef IS64
		if (GAME_VER == 6156) {
			ip = getField(int, &server, 0x80);
			port = (int)getField(unsigned short, &server, 0x84);
		}
		else if (GAME_VER == 5767) {
			ip = getField(int, &server, 0x30);
			port = (int)getField(unsigned short, &server, 0x34);
		}
#else
		if (GAME_VER == 5767) {
			ip = (int)getField(int, &server, 0x14);
			port = (int)getField(int, &server, 0x18);
		}
#endif
		sprintf(sz_ip, "%d.%d.%d.%d", (ip) & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
		IScriptSystem *pScriptSystem = pSystem->GetIScriptSystem();
		pScriptSystem->BeginCall("CheckSelectedServer");
		pScriptSystem->PushFuncParam(sz_ip);
		pScriptSystem->PushFuncParam(port);
		if (GAME_VER == 6156)
			pScriptSystem->PushFuncParam("<unknown map>");
		pScriptSystem->EndCall();
	}
	return result;
}
void __fastcall JoinServer(void *self, void *addr) {
	checkFollowing = true;
	unhook((void*)pJoinServer);
	pJoinServer(self, addr);
	hook((void*)pJoinServer, (void*)JoinServer);
	checkFollowing = false;
}
void __fastcall DisconnectError(void *self, void *addr, EDisconnectionCause dc, bool connecting, const char* serverMsg) {
	if (dc == eDC_MapNotFound || dc == eDC_MapVersion) {
		IScriptSystem *pScriptSystem = pSystem->GetIScriptSystem();
		pScriptSystem->BeginCall("TryDownloadFromRepo");
		pScriptSystem->PushFuncParam(serverMsg);
		pScriptSystem->EndCall();
	}
	//unhook(pDisconnectError);
	pDisconnectError(self, addr, dc, connecting, serverMsg);
	//hook((void*)pDisconnectError, (void*)DisconnectError);
}
int __fastcall GameUpdate(void* self, void *addr, bool p1, unsigned int p2) {
	//unhook(pGameUpdate);
	//int res = pGameUpdate(self, addr, p1, p2);
	//hook((void*)pGameUpdate, (void*)GameUpdate);
	OnUpdate(0.0f);
	return pGameUpdate(self, addr, p1, p2);
}

void OnUpdate(float frameTime) {
	bool eventFinished = false;

	for (int i = 0; g_objectsInQueue && i < MAX_ASYNC_QUEUE; i++) {
		g_mutex.Lock();
		AsyncData *obj = asyncQueue[i];
		if (obj) {
			if (obj->finished) {
				try {
					obj->postExec();
				}
				catch (std::exception& ex) {
					printf("postfn/Unhandled exception: %s", ex.what());
				}
				try {
					delete obj;
				}
				catch (std::exception& ex) {
					printf("delete/Unhandled exception: %s", ex.what());
				}
				eventFinished = true;
				g_objectsInQueue--;
				asyncQueue[i] = 0;
			} else if (obj->executing) {
				try {
					obj->onUpdate();
				}
				catch (std::exception& ex) {
					printf("progress_func/Unhandled exception: %s", ex.what());
				}
			}
		}
		g_mutex.Unlock();
	}
	static unsigned int localCounter = 0;
	if (eventFinished
#ifndef MAX_PERFORMANCE
		|| ((localCounter & 3) == 0)
#endif
		) {	//loop every fourth cycle to save some performance
		IScriptSystem *pScriptSystem = pSystem->GetIScriptSystem();
		pScriptSystem->BeginCall("OnUpdate");
		pScriptSystem->PushFuncParam(frameTime);
		pScriptSystem->EndCall();
	}
	localCounter++;
}

void MemScan(void *base,int size){
	char buffer[81920]="";
	for(int i=0;i<size;i++){
		if(i%16==0) sprintf(buffer,"%s %#04X: ",buffer,i);
		sprintf(buffer,"%s %02X",buffer,((char*)base)[i]&0xFF);
		if(i%16==15) sprintf(buffer,"%s\n",buffer);
	}
	MessageBoxA(0,buffer,0,0);
}

void* __stdcall Hook_GetHostByName(const char* name){
	unhook(gethostbyname);
	hostent *h=0;
	if(strcmp(SvMaster,"gamespy.com")){
		int len=strlen(name);
		char *buff=new char[len+256];
		strncpy(buff,name, len+256);
		int a,b,c,d;
		bool isip = sscanf(SvMaster,"%d.%d.%d.%d",&a,&b,&c,&d) == 4;
		if(char *ptr=strstr(buff,"gamespy.com")){
			if(!isip)
				memcpy(ptr,SvMaster,strlen(SvMaster));
		}
		else if(char *ptr=strstr(buff,"gamesspy.eu")){
			if(!isip)
				memcpy(ptr,SvMaster,strlen(SvMaster));
		}
		h = gethostbyname(buff);
		delete [] buff;
	} else {
		h = gethostbyname(name);
	}
	hook(gethostbyname,Hook_GetHostByName);
	return h;
}
bool TestFileWrite(const char *path) {
	FILE *f = fopen(path, "w+");
	if (f) {
		fputs("test", f);
		int sz = ftell(f);
		fclose(f);
		if (sz > 0) return true;
	}
	return false;
}
bool TestGameFilesWritable() {
	char cwd[5120];
	GetModuleFileNameA(0, cwd, 5120);
	int last = -1;
	for (int i = 0, j = strlen(cwd); i<j; i++) {
		if (cwd[i] == '\\')
			last = i;
	}
	if (last >= 0)
		cwd[last] = 0;
	char params[5120];
	char gameDir[5120];
	sprintf(gameDir, "%s\\..\\Game\\Levels\\_write.dat", cwd);
	if (TestFileWrite(gameDir)) return true;
	sprintf(cwd, "%s\\..\\SfwClFiles\\", cwd);
	SHELLEXECUTEINFOA info;
	ZeroMemory(&info, sizeof(SHELLEXECUTEINFOA));
	info.lpDirectory = cwd;
	info.lpParameters = params;
	info.lpFile = "sfwcl_precheck.exe";
	info.nShow = SW_HIDE;
	info.cbSize = sizeof(SHELLEXECUTEINFOA);
	info.fMask = SEE_MASK_NOCLOSEPROCESS;
	info.hwnd = 0;
	if (ShellExecuteExA(&info)) {
		return TestFileWrite(gameDir);
	} else return false;
}

BOOL APIENTRY DllMain(HANDLE,DWORD,LPVOID){
	return TRUE;
}
inline void fillNOP(void *addr,int l){
	DWORD tmp=0;
	VirtualProtect(addr,l*2,PAGE_READWRITE,&tmp);
	memset(addr,'\x90',l);
	VirtualProtect(addr,l*2,tmp,&tmp);
}
extern "C" {
	__declspec(dllexport) void patchMem(int ver){
		switch(ver){
#ifdef IS64
			case 5767:
				fillNOP((void*)0x3968C719,6);
				fillNOP((void*)0x3968C728,6);

				pShowLoginScreen=(PFNSHLS)0x39308250;
				hook((void*)pShowLoginScreen,(void*)OnShowLoginScr);

				pJoinServer=(PFNSHLS)0x3931F3E0;
				hook((void*)pJoinServer,(void*)JoinServer);

				pGetSelectedServer=(PFNGSS)0x39313C40;
				hook((void*)pGetSelectedServer,(void*)GetSelectedServer);

				break;
			case 6156:
				fillNOP((void*)0x39689899,6);
				fillNOP((void*)0x396898A8,6);

				pGetMenu = (PFNGM)0x390BB910;
				pGetMenuScreen = (PFNGMS)0x392F04B0;
				pMenuIsLoaded = (PFNMIL)0x39340220;

				//pGameUpdate = (PFNGU)0x390BB5F0;
				pGameUpdate = (PFNGU)hookp((void*)0x390BB5F0, (void*)GameUpdate, 15);

				pShowLoginScreen=(PFNSHLS)0x393126B0;
				hook((void*)pShowLoginScreen,(void*)OnShowLoginScr);

				pJoinServer=(PFNSHLS)0x3932C090;
				hook((void*)pJoinServer,(void*)JoinServer);

				pGetSelectedServer=(PFNGSS)0x39320D60;
				hook((void*)pGetSelectedServer,(void*)GetSelectedServer);

				//pDisconnectError=(PFNDE)0x39315EB0; 
				//hook((void*)pDisconnectError,(void*)DisconnectError);
				pDisconnectError = (PFNDE)hookp((void*)0x39315EB0, (void*)DisconnectError, 12);
				break;
			case 6729:
				fillNOP((void*)0x3968B0B9,6);
				fillNOP((void*)0x3968B0C8,6);
				break;
#else
			case 5767:
				fillNOP((void*)0x3953F4B7,2);
				fillNOP((void*)0x3953F4C0,2);

				pShowLoginScreen=(PFNSHLS)0x3922A330;
				hook((void*)pShowLoginScreen,(void*)OnShowLoginScr);

				pJoinServer=(PFNSHLS)0x39234E50;
				hook((void*)pJoinServer,(void*)JoinServer);

				pGetSelectedServer=(PFNGSS)0x3922E650;
				hook((void*)pGetSelectedServer,(void*)GetSelectedServer);

				break;
			case 6156:
				fillNOP((void*)0x3953FB7E,2);
				fillNOP((void*)0x3953FB87,2);

				pGetMenu = (PFNGM)0x390B5CA0;
				pGetMenuScreen = (PFNGMS)0x3921D310;
				pMenuIsLoaded = (PFNMIL)0x39249410;

				pShowLoginScreen=(PFNSHLS)0x39230E00;
				hook((void*)pShowLoginScreen,(void*)OnShowLoginScr);

				//pGameUpdate = (PFNGU)0x390B5A40;
				pGameUpdate = (PFNGU)hookp((void*)0x390B5A40, (void*)GameUpdate, 7);

				pJoinServer=(PFNSHLS)0x3923D820;
				hook((void*)pJoinServer,(void*)JoinServer);

				pGetSelectedServer=(PFNGSS)0x3923BB70;
				hook((void*)pGetSelectedServer,(void*)GetSelectedServer);

				//pDisconnectError=(PFNDE)0x39232D90;
				//hook((void*)pDisconnectError,(void*)DisconnectError);
				pDisconnectError = (PFNDE)hookp((void*)0x39232D90, (void*)DisconnectError, 7);
				break;
			case 6729:
				fillNOP((void*)0x3953FF89,2);
				fillNOP((void*)0x3953FF92,2);

				pShowLoginScreen=(PFNSHLS)0x39230E00;
				hook((void*)pShowLoginScreen,(void*)OnShowLoginScr);

				break;
#endif
		}
	}
	__declspec(dllexport) void* CreateGame(void* ptr){

#ifdef AUTO_UPDATE
		bool needsUpdate = false;
		std::string newestVersion = fastDownload(
			(
				std::string("http://crymp.net/dl/version.txt?")
				+(std::to_string(time(0)))
			).c_str()
		);
		if(atoi(newestVersion.c_str())!=CLIENT_BUILD && GetLastError()==0){
			//MessageBoxA(0,newestVersion.c_str(),0,MB_OK);
			if(autoUpdateClient()){
				TerminateProcess(GetCurrentProcess(),0);
				::PostQuitMessage(0);
				return 0;
			}
		}
#endif
		typedef void* (*PFNCREATEGAME)(void*);
		int version=getGameVer(".\\.\\.\\Bin32\\CryGame.dll");
#ifdef IS64
		HMODULE lib=LoadLibraryA(".\\.\\.\\Bin64\\CryGame.dll");
		hMapDlLib = LoadLibraryA(".\\.\\.\\Mods\\sfwcl\\Bin64\\MapDownloader.dll");
#else
		HMODULE lib=LoadLibraryA(".\\.\\.\\Bin32\\CryGame.dll");
		hMapDlLib = LoadLibraryA(".\\.\\.\\Mods\\sfwcl\\Bin32\\MapDownloader.dll");
#endif
		PFNCREATEGAME createGame=(PFNCREATEGAME)GetProcAddress(lib,"CreateGame");
		if (hMapDlLib) {
			pfnSetUpdateProgressCallback = (PFNSETUPDATEPROGRESSCALLBACK)GetProcAddress(hMapDlLib, "SetUpdateProgressCallback");
			pfnDownloadMap = (PFNDOWNLOADMAP)GetProcAddress(hMapDlLib, "DownloadMap");
			pfnSetUpdateProgressCallback((void*)MapDownloadUpdateProgress);
		} else {
			char msg[100];
			sprintf(msg, "FAILED TO LOAD MAPDL: %d", GetLastError());
			MessageBoxA(0, msg, 0, 0);
		}
		pGame=(IGame*)createGame(ptr);
		GAME_VER=version;
		patchMem(version);
		hook(gethostbyname,Hook_GetHostByName);
		g_gameFilesWritable = TestGameFilesWritable();

		pGameFramework=(IGameFramework*)ptr;
		pSystem=pGameFramework->GetISystem();
		pScriptSystem=pSystem->GetIScriptSystem();
		pConsole=pSystem->GetIConsole();
		pConsole->AddCommand("cl_master",CommandClMaster,VF_RESTRICTEDMODE);
		pConsole->AddCommand("reload_maps",CommandRldMaps,VF_RESTRICTEDMODE);

		pScriptSystem->SetGlobalValue("GAME_VER",version);
#ifdef MAX_PERFORMANCE
		pScriptSystem->SetGlobalValue("MAX_PERFORMANCE", true);
#endif
		if(!luaApi)
			luaApi=new CPPAPI(pSystem,pGameFramework);
		if(!socketApi)
			socketApi=new Socket(pSystem,pGameFramework);

		return pGame;
	}
}
