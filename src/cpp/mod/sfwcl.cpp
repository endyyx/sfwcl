/*#include <MsXml2.h>
#include <ExDisp.h>	//InternetExplorer!*/
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

//std::mutex commonMutex;
#ifdef USE_SDK
	typedef unsigned int uint;
	#include <IGameFramework.h>
	#include <ISystem.h>
	#include <IScriptSystem.h>
	#include <IConsole.h>
	#include <IFont.h>
	#include <IUIDraw.h>
	#include <IFlashPlayer.h>
	#include <WinSock2.h>
	#pragma comment(lib,"ws2_32")
	#include "CPPAPI.h"
	#include "Socket.h"
	#include "Structs.h"
	CPPAPI *luaApi=0;
	Socket *socketApi=0;
	ISystem *pSystem=0;
	IConsole *pConsole=0;
	GAME_32_6156 *pGameGlobal=0;
	IGame *pGame = 0;
	IScriptSystem *pScriptSystem=0;
	IGameFramework *pGameFramework=0;
	IFlashPlayer *pFlashPlayer=0;
	AsyncData *asyncQueue[MAX_ASYNC_QUEUE];
	std::map<std::string, std::string> asyncRetVal;
	int asyncQueueIdx = 0;
#endif

#define getField(type,base,offset) (*(type*)(((unsigned char*)base)+offset))

typedef void (__fastcall *PFNLOGIN)(void*,void*,const char*);		//HUD::OnLogin
typedef void (__fastcall *PFNSHLS)(void*,void*);					//HUD::ShowLoginScreen
typedef void (__fastcall *PFNJS)(void*,void*);						//HUD::JoinServer
typedef bool (__fastcall *PFNGSS)(void*,void*,SServerInfo&);		//HUD::GetSelectedServer
typedef void (__fastcall *PFNDE)(void*,void*,EDisconnectionCause, bool,const char*);	//HUD::OnDisconnectError
typedef void (__fastcall *PFNSE)(void*,void*,const char*,int);		//HUD::ShowError
typedef void* (__fastcall *PFNGM)(void*, void*);					//CGame::GetMenu
typedef void* (__fastcall *PFNGMS)(void*, void*, char);				//FlashObj::GetMenuScreen
typedef bool (__fastcall *PFNMIL)(void*, void*);					//FlashScreen::IsLoaded
typedef int (__fastcall *PFNGU)(void*, void*, bool, unsigned int);	//CGame::Update


int GAME_VER=6156;

Mutex g_mutex;

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

void *m_ui;
char SvMaster[255]="m.crymp.net";

void OnUpdate(float frameTime);

#ifdef USE_SDK

void CommandClMaster(IConsoleCmdArgs *pArgs){
	if (pArgs->GetArgCount()>1)
	{
		const char *to=pArgs->GetCommandLine()+strlen(pArgs->GetArg(0))+1;
		strcpy(SvMaster, to);
	}
	pScriptSystem->BeginCall("printf");
	char buff[50];
	sprintf(buff,"$0    cl_master = $6%s",SvMaster);
	pScriptSystem->PushFuncParam("%s");
	pScriptSystem->PushFuncParam(buff);
	pScriptSystem->EndCall();
	
	//ToggleLoading("Setting master",true);
}
void CommandRldMaps(IConsoleCmdArgs *pArgs){
	ILevelSystem *pLevelSystem = pGameFramework->GetILevelSystem();
	if(pLevelSystem){
		pLevelSystem->Rescan();
	}
}
#endif

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
	DEPRECATED:
void __fastcall ShowError(void *self,void *addr,const char *msg,int code){
	if(code==eDC_MapNotFound || code==eDC_MapVersion){
		IScriptSystem *pScriptSystem=pSystem->GetIScriptSystem();
		pScriptSystem->BeginCall("finddownload");
		pScriptSystem->PushFuncParam(msg);
		pScriptSystem->EndCall();
	}
	MessageBoxA(0,msg,0,0);
	unhook(pShowError);
	pShowError(self,addr,msg,code);
	hook((void*)pShowError,(void*)ShowError);
}
void __fastcall OnLoginFailed(void *self,void *addr,const char *reason){
#ifdef USE_SDK
	pScriptSystem->BeginCall("OnLogin");
	pScriptSystem->EndCall();
#endif
	pLoginSuccess(self,addr,reason);
}
*/
void __fastcall OnShowLoginScr(void *self,void *addr){
#ifdef USE_SDK
	pScriptSystem->BeginCall("OnShowLoginScreen");
//#ifndef IS64
	pScriptSystem->PushFuncParam(true);
//#endif
	pScriptSystem->EndCall();
	pFlashPlayer=(IFlashPlayer*)*(unsigned long*)(self);
	pFlashPlayer->Invoke1("_root.Root.MainMenu.MultiPlayer.MultiPlayer.gotoAndPlay", "internetgame");
#endif
}

IFlashPlayer *GetFlashPlayer(int offset=0, int pos=-1) {
	if (!pGetMenu) return pFlashPlayer;
	void *pMenu = pGetMenu(pGame, pGetMenu);
	for (int i = offset; i < 6; i++) {
		if (pos != -1 && i != pos) continue;
		MENU_SCREEN *pMenuScreen = (MENU_SCREEN*)pGetMenuScreen(pMenu, pGetMenuScreen, i);
		if (pMenuScreen && pMenuIsLoaded(pMenuScreen, pMenuIsLoaded)) {
			return (IFlashPlayer*)pMenuScreen->PTR1;
		}
	}
	return 0;
}

void ToggleLoading(const char *text,bool loading,bool reset){
	static bool isActive = false;
	if (loading && isActive) {
		reset = false;
	}
	isActive = loading;
	pFlashPlayer = GetFlashPlayer();
	if (pFlashPlayer) {
		if(reset)
			pFlashPlayer->Invoke1("showLOADING", loading);
		if (loading) {
			SFlashVarValue args[] = { text,false };
			pFlashPlayer->Invoke("setLOADINGText", args, sizeof(args) / sizeof(args[0]));
		}
	}
}

void* __stdcall Hook_GetHostByName(const char* name){
	unhook(gethostbyname);
	hostent *h=0;
	if(strcmp(SvMaster,"gamespy.com")){
		int len=strlen(name);
		char *buff=new char[len+255];
		strcpy(buff,name);
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
		h=gethostbyname(buff);
		delete [] buff;
	} else {
		h=gethostbyname(name);
	}
	hook(gethostbyname,Hook_GetHostByName);
	return h;
}

bool checkFollowing=false;
bool __fastcall GetSelectedServer(void *self, void *addr, SServerInfo& server){
#ifdef USE_SDK
	m_ui=self;
	unhook(pGetSelectedServer);
	bool result=pGetSelectedServer(self,addr,server);
	hook(pGetSelectedServer,GetSelectedServer);
	if (checkFollowing) {
		char sz_ip[30];
		int ip = server.m_publicIP;
		int port = server.m_publicPort;
#ifdef IS64
		if (GAME_VER == 6156) {
			ip = getField(int, &server, 0x80);
			port = (int)getField(unsigned short, &server, 0x84);
		} else if (GAME_VER == 5767) {
			ip = getField(int, &server, 0x30);
			port = (int)getField(unsigned short, &server, 0x34);
		}
#else
		if (GAME_VER == 5767) {
			ip = (int)getField(int, &server, 0x14);
			port = (int)getField(int, &server, 0x18);
		}
#endif
		//MemScan(&server, 256);
		sprintf(sz_ip,"%d.%d.%d.%d",(ip)&0xFF,(ip>>8)&0xFF,(ip>>16)&0xFF,(ip>>24)&0xFF);
		//ToggleLoading("Checking server",true);
		IScriptSystem *pScriptSystem=pSystem->GetIScriptSystem();
		pScriptSystem->BeginCall("CheckSelectedServer");
		pScriptSystem->PushFuncParam(sz_ip);
		pScriptSystem->PushFuncParam(port);
		if(GAME_VER==6156)
			pScriptSystem->PushFuncParam("<unknown map>");
		pScriptSystem->EndCall();
	}
	return result;
#endif
}
void __fastcall JoinServer(void *self,void *addr){
#ifdef USE_SDK
	checkFollowing=true;
	unhook((void*)pJoinServer);
	pJoinServer(self,addr);
	hook((void*)pJoinServer,(void*)JoinServer);
	checkFollowing=false;
#endif
}
void __fastcall DisconnectError(void *self, void *addr,EDisconnectionCause dc, bool connecting, const char* serverMsg){
	if(dc==eDC_MapNotFound || dc==eDC_MapVersion){
		IScriptSystem *pScriptSystem=pSystem->GetIScriptSystem();
		pScriptSystem->BeginCall("TryDownloadFromRepo");
		pScriptSystem->PushFuncParam(serverMsg);
		pScriptSystem->EndCall();
	}
	unhook(pDisconnectError);
	pDisconnectError(self,addr,dc,connecting,serverMsg);
	hook((void*)pDisconnectError,(void*)DisconnectError);
}
int __fastcall GameUpdate(void* self, void *addr, bool p1, unsigned int p2) {
	unhook(pGameUpdate);
	int res = pGameUpdate(self, addr, p1, p2);
	hook((void*)pGameUpdate, (void*)GameUpdate);
	OnUpdate(0.0f);
	return res;
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
#ifdef USE_SDK
int OnImpulse( const EventPhys *pEvent ){
#ifdef WANT_CIRCLEJUMP
	return 1;
#else
	return 0;
#endif
}
#endif

void OnUpdate(float frameTime) {
	for (int i = 0; i < MAX_ASYNC_QUEUE; i++) {
		g_mutex.Lock();
		AsyncData *obj = asyncQueue[i];
		if (obj) {
			if (obj->finished) {
				try {
					obj->postExec();
				} catch (std::exception& ex) {
					printf("postfn/Unhandled exception: %s", ex.what());
				}
				try {
					delete obj;
				} catch (std::exception& ex) {
					printf("delete/Unhandled exception: %s", ex.what());
				}
				asyncQueue[i] = 0;
			} else if (obj->executing) {
				try {
					obj->onUpdate();
				} catch (std::exception& ex) {
					printf("progress_func/Unhandled exception: %s", ex.what());
				}
			}
		}
		g_mutex.Unlock();
	}
	IScriptSystem *pScriptSystem = pSystem->GetIScriptSystem();
	pScriptSystem->BeginCall("OnUpdate");
	pScriptSystem->PushFuncParam(frameTime);
	pScriptSystem->EndCall();
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
				pGameGlobal = (GAME_32_6156*)0x3941C858;
				pGetMenu = (PFNGM)0x390BB910;
				pGetMenuScreen = (PFNGMS)0x392F04B0;
				pMenuIsLoaded = (PFNMIL)0x39340220;

				pGameUpdate = (PFNGU)0x390BB5F0;
				hook((void*)pGameUpdate, (void*)GameUpdate);

				pShowLoginScreen=(PFNSHLS)0x393126B0;
				hook((void*)pShowLoginScreen,(void*)OnShowLoginScr);

				pJoinServer=(PFNSHLS)0x3932C090;
				hook((void*)pJoinServer,(void*)JoinServer);

				pGetSelectedServer=(PFNGSS)0x39320D60;
				hook((void*)pGetSelectedServer,(void*)GetSelectedServer);

				pDisconnectError=(PFNDE)0x39315EB0; 
				hook((void*)pDisconnectError,(void*)DisconnectError);
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

				//pLoginSuccess=(PFNLOGIN)0x3922C090;
				//hookp((void*)0x3922C170,(void*)OnLoginFailed,6);

				//pShowError=(PFNSE)0x3922A7F0;
				//hook((void*)pShowError,(void*)ShowError);
				break;
			case 6156:
				fillNOP((void*)0x3953FB7E,2);
				fillNOP((void*)0x3953FB87,2);
				pGameGlobal = (GAME_32_6156*)0x392A6FCC;
				pGetMenu = (PFNGM)0x390B5CA0;
				pGetMenuScreen = (PFNGMS)0x3921D310;
				pMenuIsLoaded = (PFNMIL)0x39249410;

				pShowLoginScreen=(PFNSHLS)0x39230E00;
				hook((void*)pShowLoginScreen,(void*)OnShowLoginScr);

				pGameUpdate = (PFNGU)0x390B5A40;
				hook((void*)pGameUpdate, (void*)GameUpdate);

				pJoinServer=(PFNSHLS)0x3923D820;
				hook((void*)pJoinServer,(void*)JoinServer);

				pGetSelectedServer=(PFNGSS)0x3923BB70;
				hook((void*)pGetSelectedServer,(void*)GetSelectedServer);

				pDisconnectError=(PFNDE)0x39232D90;
				hook((void*)pDisconnectError,(void*)DisconnectError);

				//pLoginSuccess=(PFNLOGIN)0x39232B30;
				//hookp((void*)0x39232C10,(void*)OnLoginFailed,6);
				break;
			case 6729:
				fillNOP((void*)0x3953FF89,2);
				fillNOP((void*)0x3953FF92,2);
				
				pShowLoginScreen=(PFNSHLS)0x39230E00;
				hook((void*)pShowLoginScreen,(void*)OnShowLoginScr);
				//pLoginSuccess=(PFNLOGIN)0x392423A0;
				//hookp((void*)0x3923F8C0,(void*)OnLoginFailed,6);
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
#else
		HMODULE lib=LoadLibraryA(".\\.\\.\\Bin32\\CryGame.dll");
#endif
		PFNCREATEGAME createGame=(PFNCREATEGAME)GetProcAddress(lib,"CreateGame");
		pGame=(IGame*)createGame(ptr);
		GAME_VER=version;
		patchMem(version);
		hook(gethostbyname,Hook_GetHostByName);
#if !defined(USE_SDK)
	#ifndef IS64
		void *pSystem=0;
		void *pScriptSystem=0;
		void *tmp=0;
		const char *idx="GAME_VER";
		float ver=version;
		mkcall(pSystem,ptr,64);
		mkcall(pScriptSystem,pSystem,108);
		mkcall2(tmp,pScriptSystem,84,idx,&ver);
	#endif
#else
		pGameFramework=(IGameFramework*)ptr;
		pSystem=pGameFramework->GetISystem();
		pScriptSystem=pSystem->GetIScriptSystem();
		pConsole=pSystem->GetIConsole();
		pConsole->AddCommand("cl_master",CommandClMaster);
		pConsole->AddCommand("reload_maps",CommandRldMaps);
#ifdef WANT_CIRCLEJUMP
		IPhysicalWorld *pPhysicalWorld=pSystem->GetIPhysicalWorld();
		pPhysicalWorld->AddEventClient( 1,OnImpulse,0 );  
#endif
		pScriptSystem->SetGlobalValue("GAME_VER",version);
		if(!luaApi)
			luaApi=new CPPAPI(pSystem,pGameFramework);
		if(!socketApi)
			socketApi=new Socket(pSystem,pGameFramework);
#endif
		return pGame;
	}
}