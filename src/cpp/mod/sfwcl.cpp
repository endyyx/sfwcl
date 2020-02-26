#include "Shared.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sstream>
#include "Mutex.h"
#include "Protect.h"
#include "RPC.h"
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
#include <IGameRulesSystem.h>
#include <IFont.h>
#include <IUIDraw.h>
#include <IFlashPlayer.h>
#include <WinSock2.h>
#include <ShlObj.h>
#include "CPPAPI.h"
#include "Socket.h"
#include "Structs.h"
#include "IntegrityService.h"
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
RPC *rpc = 0;
//AsyncData *asyncQueue[MAX_ASYNC_QUEUE+1];
std::list<AsyncData*> asyncQueue;
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
typedef bool(__fastcall *PFNHFSC)(void*, void*, const char*, const char*); //MPHub::HandleFSCommand

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
PFNHFSC pHandleFSCommand = 0;
PFNSETUPDATEPROGRESSCALLBACK pfnSetUpdateProgressCallback = 0;
PFNDOWNLOADMAP pfnDownloadMap = 0;
PFNCANCELDOWNLOAD pfnCancelDownload = 0;

HMODULE hMapDlLib = 0;

void *m_ui;
char SvMaster[11];

bool TestGameFilesWritable();
void OnUpdate(float frameTime);
void MemScan(void *base, int size);

void __stdcall MapDownloadUpdateProgress(const char *msg, bool error) {
	mapDlMessage.set(msg);
}

void InitGameObjects();

bool LoadScript(const char *name) {
	char *main = 0;
	int len = FileDecrypt(name, &main);
	if (len) {
		bool ok = true;
		if (pScriptSystem) {
			if (!pScriptSystem->ExecuteBuffer(main, len)) {
				ok = false;
			}
		}
		for (int i = 0; i < len; i++) main[i] = 0;
		delete[] main;
		main = 0;
		return ok;
	}
	return false;
}
void InitScripts() {
#ifdef PRERELEASE_BUILD
	FileEncrypt("Files\\Main.lua", "Files\\Main.bin");
	FileEncrypt("Files\\GameRules.lua", "Files\\GameRules.bin");
	FileEncrypt("Files\\IntegrityService.lua", "Files\\IntegrityService.bin");
#endif
	if (!LoadScript("Files\\Main.bin")) {
		//pSystem->Quit();
	}
	if (!LoadScript("Files\\IntegrityService.bin")) {
		//pSystem->Quit();
	}
}
void PostInitScripts() {
	ScriptAnyValue a;
	if (pScriptSystem->GetGlobalAny("g_gameRules", a) && a.table && a.GetVarType() == ScriptVarType::svtObject) {
		bool v = false;
		a.table->AddRef();
		if (!a.table->GetValue("IsModified", v)) {
			if (!LoadScript("Files\\GameRules.bin")) {
				pSystem->Quit();
			}
		}
		a.table->Release();
	}
}

char* gsptrs[14]; // array of gamespy strings (addresses), populated in CreateGame
				  // 15 references to gamespy.com in CryNetwork.dll, but gamestats.gamespy.com is not used so there are 14 addresses to patch
				  // gamespy.net addresses do not need to be patched, those are passed as SOAP data and used by gamespy competition server, maybe useless tho

void PatchGSHostnames() {
	const int sz = sizeof("gamespy.com") - 1; //ignore NULL byte
	for (int i = 0; i < sizeof(gsptrs) / sizeof(char*); i++) {
		DWORD fl = 0;
		VirtualProtect(gsptrs[i], sz, PAGE_READWRITE, &fl);
		strncpy(gsptrs[i], SvMaster, sz);
		VirtualProtect(gsptrs[i], sz, fl, &fl);
	}
}

void CommandClMaster(IConsoleCmdArgs *pArgs){
	if (pArgs->GetArgCount()>1)
	{
		const char *to=pArgs->GetCommandLine()+strlen(pArgs->GetArg(0))+1;
		strncpy(SvMaster, to, sizeof(SvMaster));
		PatchGSHostnames();
	}

	if (pScriptSystem->BeginCall("printf")) {
		char buff[50];
		sprintf(buff, "$0    cl_master = $6%s", SvMaster);
		pScriptSystem->PushFuncParam("%s");
		pScriptSystem->PushFuncParam(buff);
		pScriptSystem->EndCall();
	}
}
void CommandRldMaps(IConsoleCmdArgs *pArgs){
	ILevelSystem *pLevelSystem = pGameFramework->GetILevelSystem();
	if(pLevelSystem){
		pLevelSystem->Rescan();
	}
}

void __fastcall OnShowLoginScr(void *self, void *addr) {
	if (pScriptSystem->BeginCall("OnShowLoginScreen")) {
		pScriptSystem->PushFuncParam(true);
		pScriptSystem->EndCall();
	}
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
		//MemScan(&server, 1024);
#ifdef IS64
		if (GAME_VER == 6156) {
			unsigned char b = getField(unsigned char, &server, 0x38);
			int off1 = 0x80;
			int off2 = 0x84;
			if (b != 0xFE) {
				off1 += 0x40;
				off2 += 0x40;
			}
			ip = getField(int, &server, off1);
			port = (int)getField(unsigned short, &server, off2);
			port &= 0xFFFF;
		}
		else if (GAME_VER == 5767) {
			unsigned char b = getField(unsigned char, &server, 0x38);
			int off1 = 0x80;
			int off2 = 0x84;
			if (b != 0xFE) {
				off1 += 0x40;
				off2 += 0x40;
			}
			ip = getField(int, &server, off1);
			port = (int)getField(unsigned short, &server, off2);
			port &= 0xFFFF;
			//MemScan(&server, 0x100);
		}
#else
		if (GAME_VER == 5767) {
			ip = (int)getField(int, &server, 0x14);
			port = (int)getField(int, &server, 0x18);
			port &= 0xFFFF;
		}
#endif
		sprintf(sz_ip, "%d.%d.%d.%d", (ip) & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
		IScriptSystem *pScriptSystem = pSystem->GetIScriptSystem();
		if (pScriptSystem->BeginCall("CheckSelectedServer")) {
			pScriptSystem->PushFuncParam(sz_ip);
			pScriptSystem->PushFuncParam(port);

			if (GAME_VER == 6156)
				pScriptSystem->PushFuncParam("<unknown map>");
			pScriptSystem->EndCall();
		}
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
		if (pScriptSystem->BeginCall("TryDownloadFromRepo")) {
			pScriptSystem->PushFuncParam(serverMsg);
			pScriptSystem->EndCall();
		}
	} else if (rpc) rpc->shutdown();
	pDisconnectError(self, addr, dc, connecting, serverMsg);
}
bool __fastcall HandleFSCommand(void *self, void *addr, const char *pCmd, const char *pArgs) {
	IScriptSystem *pScriptSystem = pSystem->GetIScriptSystem();
	if (pScriptSystem->BeginCall("HandleFSCommand")) {
#ifdef IS64
		if (pArgs)
			pScriptSystem->PushFuncParam(pArgs - 1);
		if (pCmd)
			pScriptSystem->PushFuncParam(pCmd);
#else
		if (pCmd)
			pScriptSystem->PushFuncParam(pCmd);
		if (pArgs)
			pScriptSystem->PushFuncParam(pArgs);
#endif
		pScriptSystem->EndCall();
	}
	return pHandleFSCommand(self, addr, pCmd, pArgs);
}
int __fastcall GameUpdate(void* self, void *addr, bool p1, unsigned int p2) {
	PostInitScripts();
	OnUpdate(0.0f);
	return pGameUpdate(self, addr, p1, p2);
}

void OnUpdate(float frameTime) {
	bool eventFinished = false;

	static bool firstRun = true;
	if (firstRun) {
		firstRun = false;
		InitGameObjects();
	}

	for (std::list<AsyncData*>::iterator it = asyncQueue.begin(); g_objectsInQueue && it != asyncQueue.end(); it++) {
		g_mutex.Lock();
		AsyncData *obj = *it;
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
				asyncQueue.erase(it);
				it--;
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
		if (pScriptSystem->BeginCall("OnUpdate")) {
			pScriptSystem->PushFuncParam(frameTime);
			pScriptSystem->EndCall();
		}
	}
	localCounter++;
}

void InitGameObjects() {
	REGISTER_GAME_OBJECT(pGameFramework, IntegrityService, "Scripts/Entities/Environment/Shake.lua");
	if (pScriptSystem->BeginCall("InitGameObjects")) {
		pScriptSystem->EndCall();
	}
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
/*
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
	//MessageBox(NULL, name, NULL, NULL);
	hook(gethostbyname,Hook_GetHostByName);
	return h;
}*/
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
	char cwd[MAX_PATH], params[2 * MAX_PATH], gameDir[2 * MAX_PATH];
	getGameFolder(cwd);
	sprintf(gameDir, "%s\\Game\\Levels\\_write.dat", cwd);
	if (TestFileWrite(gameDir)) return true;
	sprintf(cwd, "%s\\SfwClFiles\\", cwd);
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


				pGameUpdate = (PFNGU)hookp((void*)0x390B8A40, (void*)GameUpdate, 15);

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
				//hook((void*)pDisconnectError,(void*)DisconnectError);                          |
				pDisconnectError = (PFNDE)hookp((void*)0x39315EB0, (void*)DisconnectError, 12);
				pHandleFSCommand = (PFNHFSC)hookp((void*)0x39318560, (void*)HandleFSCommand, 12);
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

				pGameUpdate = (PFNGU)hookp((void*)0x390B3EB0, (void*)GameUpdate, 7);

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
				pHandleFSCommand = (PFNHFSC)hookp((void*)0x39233C50, (void*)HandleFSCommand, 14);
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
		hMapDlLib = LoadLibraryA(".\\.\\.\\Mods\\sfwcl\\Bin64\\mapdl.dll");
#else
		HMODULE lib=LoadLibraryA(".\\.\\.\\Bin32\\CryGame.dll");
		hMapDlLib = LoadLibraryA(".\\.\\.\\Mods\\sfwcl\\Bin32\\mapdl.dll");
#endif
		PFNCREATEGAME createGame=(PFNCREATEGAME)GetProcAddress(lib,"CreateGame");
		if (hMapDlLib) {
			pfnSetUpdateProgressCallback = (PFNSETUPDATEPROGRESSCALLBACK)GetProcAddress(hMapDlLib, "SetUpdateProgressCallback");
			pfnDownloadMap = (PFNDOWNLOADMAP)GetProcAddress(hMapDlLib, "DownloadMap");
			pfnSetUpdateProgressCallback((void*)MapDownloadUpdateProgress);
			pfnCancelDownload = (PFNCANCELDOWNLOAD)GetProcAddress(hMapDlLib, "CancelDownload");
		}
		pGame=(IGame*)createGame(ptr);
		GAME_VER=version;
		patchMem(version);
		// no more gethostbyname hooking, patch hostname string directly
		//hook(gethostbyname,Hook_GetHostByName);

		// here we fill the array containing pointers to gamespy hostnames
		switch (GAME_VER)
		{
#ifndef IS64
		case 6156:
			gsptrs[0] = (char*)(0x395C58A8 + 8); // %s.ms%d.gamespy.com
			gsptrs[1] = (char*)(0x395C7878 + 15); // http://%s.sake.gamespy.com/SakeStorageServer/StorageServer.asmx
			gsptrs[2] = (char*)(0x395C630C + 9); // peerchat.gamespy.com
			gsptrs[3] = (char*)(0x395C8670 + 23); // https://%s.auth.pubsvs.gamespy.com/AuthService/AuthService.asmx
			gsptrs[4] = (char*)(0x395CF780 + 5); // gpcm.gamespy.com
			gsptrs[5] = (char*)(0x395CF638 + 5); // gpsp.gamespy.com
			gsptrs[6] = (char*)(0x395C5558 + 8); // natneg1.gamespy.com
			gsptrs[7] = (char*)(0x395C556C + 8); // natneg2.gamespy.com
			gsptrs[8] = (char*)(0x395C5580 + 8); // natneg3.gamespy.com
			gsptrs[9] = (char*)(0x395C58E4 + 10); // %s.master.gamespy.com
			gsptrs[10] = (char*)(0x395C458C + 13); // %s.available.gamespy.com
			gsptrs[11] = (char*)(0x395C7F70 + 12); // http://motd.gamespy.com/motd/motd.asp
			gsptrs[12] = (char*)(0x395C7F08 + 12); // http://motd.gamespy.com/motd/vercheck.asp
			gsptrs[13] = (char*)(0x395C8398 + 22); // http://%s.comp.pubsvs.gamespy.com/CompetitionService/CompetitionService.asmx

			break;
		case 5767:
			break;
		case 6729:
			break;
#else
#endif
		}

		strncpy(SvMaster, "m.crymp.net", sizeof(SvMaster));
		PatchGSHostnames();
		

		g_gameFilesWritable = true; // lets pretend installer solved it for us!! TestGameFilesWritable();


		pGameFramework=(IGameFramework*)ptr;
		pSystem=pGameFramework->GetISystem();
		pScriptSystem=pSystem->GetIScriptSystem();
		pConsole=pSystem->GetIConsole();
		pConsole->AddCommand("cl_master",CommandClMaster,VF_RESTRICTEDMODE, "Patch GameSpy master server hostname in memory");
		pConsole->AddCommand("reload_maps", CommandRldMaps, VF_RESTRICTEDMODE);

		InitScripts();

		pScriptSystem->SetGlobalValue("GAME_VER",version);
#ifdef _WIN64
		extern long PROTECT_FLAG;
		pScriptSystem->SetGlobalValue("__DUMMY0__", PROTECT_FLAG);
#endif
#ifdef MAX_PERFORMANCE
		pScriptSystem->SetGlobalValue("MAX_PERFORMANCE", true);
#endif

		if(!luaApi)
			luaApi=new CPPAPI(pSystem,pGameFramework);
		if(!socketApi)
			socketApi=new Socket(pSystem,pGameFramework);
		if (!rpc)
			rpc = new RPC();

		return pGame;
	}
}
