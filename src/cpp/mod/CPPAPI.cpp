#include "CPPAPI.h"
#include <IEntity.h>
#include <IEntitySystem.h>
#include <IVehicleSystem.h>
#include <IGameObjectSystem.h>
#include <CryThread.h>
#include <sstream>
#include <string>
#include "AtomicCounter.h"
#include "Atomic.h"
#include "Crypto.h"
//#include <mutex>
//#include <functional>

#pragma region CPPAPI

extern std::list<AsyncData*> asyncQueue;
extern int asyncQueueIdx;
extern std::map<std::string, std::string> asyncRetVal;
extern IScriptSystem *pScriptSystem;

bool DownloadMapFromObject(DownloadMapStruct *now);

HANDLE gEvent;

CPPAPI::CPPAPI(ISystem *pSystem, IGameFramework *pGameFramework)
	:	m_pSystem(pSystem),
		m_pSS(pSystem->GetIScriptSystem()),
		m_pGameFW(pGameFramework)
{
	Init(m_pSS, m_pSystem);
	gEvent=CreateEvent(0,0,0,0);
	thread=CreateThread(0,0,(LPTHREAD_START_ROUTINE)AsyncThread,0,0,0);
	SetGlobalName("CPPAPI");
	RegisterMethods();
}
CPPAPI::~CPPAPI(){
	if(thread)
		TerminateThread(thread,0);
}
void CPPAPI::RegisterMethods(){
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CPPAPI::
	SetGlobalName("CPPAPI");
	SCRIPT_REG_TEMPLFUNC(FSetCVar,"cvar, value");
	SCRIPT_REG_TEMPLFUNC(Random,"");
	SCRIPT_REG_TEMPLFUNC(MapAvailable,"host");
	SCRIPT_REG_TEMPLFUNC(DownloadMap,"mapnm,mapurl");
	SCRIPT_REG_TEMPLFUNC(ConnectWebsite,"host, page, port, http11, timeout, methodGet");
	SCRIPT_REG_TEMPLFUNC(GetIP,"host");
	SCRIPT_REG_TEMPLFUNC(GetLocalIP,"");
	SCRIPT_REG_TEMPLFUNC(GetMapName,"");
	SCRIPT_REG_TEMPLFUNC(ApplyMaskAll,"mask,apply");
	SCRIPT_REG_TEMPLFUNC(ApplyMaskOne,"ent,mask,apply");
	SCRIPT_REG_TEMPLFUNC(AsyncConnectWebsite,"host, page, port, http11, timeout, methodGet");
	SCRIPT_REG_TEMPLFUNC(MsgBox,"text,title,mask");
	SCRIPT_REG_TEMPLFUNC(DoAsyncChecks, "");
	SCRIPT_REG_TEMPLFUNC(AsyncDownloadMap, "mapn, mapdl");
	SCRIPT_REG_TEMPLFUNC(ToggleLoading, "text, loading, reset");
	SCRIPT_REG_TEMPLFUNC(CancelDownload, "");
	SCRIPT_REG_TEMPLFUNC(SignMemory, "addr1, addr2, nonce, len, id");
}
int CPPAPI::SignMemory(IFunctionHandler *pH, const char *a1, const char *a2, const char *len, const char *nonce, const char *id) {
	std::stringstream a1s, a2s, ls, ns;
	a1s << a1;
	a2s << a2;
	ls << len;
	ns << nonce;
	std::string pa1, pa2, pl, pn;
	std::string h = "";
	while ((a1s >> pa1) && (a2s >> pa2) && (ls >> pl) && (ns >> pn)) {
		if (isdigit(pa1[0])) {
			unsigned long addr1 = 0, addr2 = 0;
			sscanf(pa1.c_str(), "%lx", &addr1);
			sscanf(pa2.c_str(), "%lx", &addr2);
#ifdef _WIN64
			unsigned long long addr = 0;
			addr |= addr1;
			addr <<= 32;
#else
			unsigned long addr = 0;
#endif
			addr |= addr2;
			void *ptr = (void*)addr;
			h += signMemory(ptr, atoi(pl.c_str()), pn.c_str(), true);
		} else if (pa1 == "file") {

		}
	}
	std::string hash = "";
	unsigned char digest[32];
	sha256((const unsigned char*)h.c_str(), h.size(), digest);
	for (int i = 0; i < 32; i++) {
		static char bf[4];
		sprintf(bf, "%02X", digest[i] & 255);
		hash += bf;
	}
	return pH->EndFunction(hash.c_str());
}
int CPPAPI::ToggleLoading(IFunctionHandler *pH, const char *text, bool loading, bool reset) {
	::ToggleLoading(text, loading, reset);
	return pH->EndFunction(true);
}
int CPPAPI::FSetCVar(IFunctionHandler* pH,const char * cvar,const char *val){
	if(ICVar *cVar=pConsole->GetCVar(cvar))
		cVar->ForceSet(val);
	return pH->EndFunction(true);
}
int CPPAPI::Random(IFunctionHandler* pH){
	static bool set=false;
	if(!set){
		srand(time(0)^clock());
		set=true;
	}
	return pH->EndFunction(rand());
}
int CPPAPI::ConnectWebsite(IFunctionHandler* pH,char * host,char * page,int port,bool http11,int timeout,bool methodGet,bool alive){
	using namespace Network; 
	std::string content="Error";
	content=Connect(host,page,methodGet?INetGet:INetPost,http11?INetHTTP11:INetHTTP10,port,timeout,alive);
	return pH->EndFunction(content.c_str());
}
int CPPAPI::GetIP(IFunctionHandler* pH,char* host){
	if(strlen(host)>0){
		char ip[255];
		Network::GetIP(host,ip);
		return pH->EndFunction(ip);
	}
	return pH->EndFunction();
}
int CPPAPI::GetLocalIP(IFunctionHandler* pH){
    char hostn[255];
    if (gethostname(hostn, sizeof(hostn))!= SOCKET_ERROR) {
        struct hostent *host = gethostbyname(hostn);
        if(host){
            for (int i = 0; host->h_addr_list[i] != 0; ++i) {
                struct in_addr addr;
                memcpy(&addr, host->h_addr_list[i], sizeof(struct in_addr));
				return pH->EndFunction(inet_ntoa(addr));
            }
        }
    }
	return pH->EndFunction();
}
int CPPAPI::GetMapName(IFunctionHandler *pH){
	return pH->EndFunction(pGameFramework->GetLevelName());
}
int CPPAPI::DoAsyncChecks(IFunctionHandler *pH) {
#ifdef DO_ASYNC_CHECKS
	extern Mutex g_mutex;
	IScriptTable *tbl = pScriptSystem->CreateTable();
	tbl->AddRef();
	std::vector<IScriptTable*> refs;
	//commonMutex.lock();
	g_mutex.Lock();
	for (std::map<std::string, std::string>::iterator it = asyncRetVal.begin(); it != asyncRetVal.end(); it++) {
		IScriptTable *item = pScriptSystem->CreateTable();
		item->AddRef();
		item->PushBack(it->first.c_str());
		item->PushBack(it->second.c_str());
		tbl->PushBack(item);
		refs.push_back(item);
	}
	g_mutex.Unlock();
	int code = pH->EndFunction(tbl);
#ifdef OLD_MSVC_DETECTED
	for (size_t i = 0; i < refs.size(); i++ ) {
		SAFE_RELEASE(refs[i]);
	}
#else
	for (auto& it : refs) {
		SAFE_RELEASE(it);
	}
#endif
	SAFE_RELEASE(tbl);
	return code;
#else
	return pH->EndFunction(0);
#endif
}
int CPPAPI::MapAvailable(IFunctionHandler *pH,const char *_path){
	char *ver=0;
	char path[255];
	strncpy(path,_path,255);
	for(int i=0,j=strlen(path);i<j;i++){
		if(path[i]=='|'){
			path[i]=0;
			ver=path+i+1;
		}
	}
	char mpath[255];
	strncpy(mpath,path,255);
	for(int i=0,j=strlen(mpath);i<j;i++)
		mpath[i]=mpath[i]=='/'?'\\':mpath[i];
	ILevelSystem *pLevelSystem = pGameFramework->GetILevelSystem();
	if(pLevelSystem){
		pLevelSystem->Rescan();
		for(int l = 0; l < pLevelSystem->GetLevelCount(); ++l){
			ILevelInfo *pLevelInfo = pLevelSystem->GetLevelInfo(l);
			if(pLevelInfo){
				if(_stricmp(pLevelInfo->GetName(),path)==0){
					bool exists=true;
					if(ver){
						char cwd[5120];
						char lpath[5120];
						GetModuleFileNameA(0,cwd,5120);
						int last=-1;
						for(int i=0,j=strlen(cwd);i<j;i++){
							if(cwd[i]=='\\')
								last=i;
						}
						if(last>=0)
							cwd[last]=0;
						sprintf(lpath,"%s\\..\\Game\\_levels.dat",cwd);
						FILE *f=fopen(lpath,"r");
						if(f){
							char name[255];
							char veri[255];
							while(!feof(f)){
								fscanf(f,"%s %s",name,veri);
								if(!strcmp(name,mpath)){
									exists=!strcmp(veri,ver);
									break;
								}
							}
							fclose(f);
						}
					}
					return pH->EndFunction(exists);
				}
			}
		}
	}
	return pH->EndFunction(false);
}
int CPPAPI::DownloadMap(IFunctionHandler *pH,const char *mapn,const char *mapdl){
	DownloadMapStruct *now=new DownloadMapStruct;
	if(now){
		//ToggleLoading("Downloading map",true);
		now->mapdl=mapdl;
		now->mapn=mapn;
		now->success = false;
		//CreateAsyncCallLua(AsyncDownloadMap,now);
		now->success = DownloadMapFromObject(now);
		return pH->EndFunction(now->success);
	}
	return pH->EndFunction();
}
int CPPAPI::CancelDownload(IFunctionHandler *pH) {
	extern PFNCANCELDOWNLOAD pfnCancelDownload;
	if (pfnCancelDownload) pfnCancelDownload();
	return pH->EndFunction();
}
int CPPAPI::AsyncConnectWebsite(IFunctionHandler* pH, char * host, char * page, int port, bool http11, int timeout, bool methodGet, bool alive) {
	using namespace Network;
	ConnectStruct *now = new ConnectStruct;
	if (now) {
		now->host = host;
		now->page = page;
		now->method = methodGet ? INetGet : INetPost;
		now->http = http11 ? INetHTTP11 : INetHTTP10;
		now->port = port;
		now->timeout = timeout;
		now->alive = alive;
		return now->callAsync(pH);
	}
	return pH->EndFunction();
}
int CPPAPI::AsyncDownloadMap(IFunctionHandler* pH, const char *path, const char *link) {
	DownloadMapStruct *now = new DownloadMapStruct;
	if (now) {
		now->mapdl = link;
		now->mapn = path;
		now->isAsync = true;
		return now->callAsync(pH);
	}
	return pH->EndFunction();
}
#pragma region Fun
int CPPAPI::ApplyMaskAll(IFunctionHandler* pH,int amask,bool apply){
	IEntitySystem *pES=pSystem->GetIEntitySystem();
	if(pES){
		IEntityIt* it=pES->GetEntityIterator();
		int c=0;
		while(!it->IsEnd()){
			IEntity *pEntity=it->This();
			int fmask=amask;
			if(fmask==MTL_LAYER_FROZEN && apply){
				IVehicleSystem *pVS=pGameFramework->GetIVehicleSystem();
				if(pVS->GetVehicle(pEntity->GetId()))
					fmask=MTL_LAYER_DYNAMICFROZEN;
			}
			IEntityRenderProxy *pRP=(IEntityRenderProxy*)pEntity->GetProxy(ENTITY_PROXY_RENDER);
			if(pRP){
				int mask=pRP->GetMaterialLayersMask();
				if(apply)
					mask|=fmask;
				else mask&=~fmask;
				pRP->SetMaterialLayersMask(mask);
			}
			c++;
			it->Next();
		}
		return pH->EndFunction(c);
	}
	return pH->EndFunction();
}
int CPPAPI::ApplyMaskOne(IFunctionHandler* pH,ScriptHandle entity,int amask,bool apply){
	IEntitySystem *pES=pSystem->GetIEntitySystem();
	if(pES){
		IEntity *pEntity=pES->GetEntity(entity.n);
		if(pEntity){
			IEntityRenderProxy *pRP=(IEntityRenderProxy*)pEntity->GetProxy(ENTITY_PROXY_RENDER);
			if(pRP){
				int mask=pRP->GetMaterialLayersMask();
				if(apply)
					mask|=amask;
				else mask&=~amask;
				pRP->SetMaterialLayersMask(mask);
				return pH->EndFunction(true);
			}
		}
	}
	return pH->EndFunction(false);
}
int CPPAPI::MsgBox(IFunctionHandler* pH,const char *text,const char *title,int buttons){
	MessageBoxA(0,text,title,buttons);
	return pH->EndFunction();
}
#pragma endregion

#pragma endregion

#pragma region AsyncStuff
#ifdef OLD_MSVC_DETECTED
BOOL WINAPI DownloadMapStructEnumProc(HWND hwnd, LPARAM lParam) {
	DownloadMapStruct::Info *pParams = (DownloadMapStruct::Info*)(lParam);
	DWORD processId;
	if (GetWindowThreadProcessId(hwnd, &processId) && processId == pParams->pid) {
		SetLastError(-1);
		pParams->hWnd = hwnd;
		return FALSE;
	}
	return TRUE;
}
#endif
bool DownloadMapFromObject(DownloadMapStruct *now) {
	IRenderer *pRend = pSystem->GetIRenderer();
	const char *mapn = now->mapn;
	const char *mapdl = now->mapdl;
	HWND hwnd = (HWND)pRend->GetHWND();
	//ShowWindow(hwnd,SW_MINIMIZE);
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
	sprintf(cwd, "%s\\..\\SfwClFiles\\", cwd);
	extern PFNDOWNLOADMAP pfnDownloadMap;
	bool ret = true;
	if (pfnDownloadMap) {
		extern Atomic<const char*> mapDlMessage;
		mapDlMessage.set(0);
		int code = pfnDownloadMap(mapn, mapdl, cwd);
		if (code) ret = false;
	} else {
		sprintf_s(params, "\"%s\" \"%s\" \"%s\"", mapn, mapdl, cwd);
		SHELLEXECUTEINFOA info;
		ZeroMemory(&info, sizeof(SHELLEXECUTEINFOA));
		info.lpDirectory = cwd;
		info.lpParameters = params;
		extern bool g_gameFilesWritable;
		///TODO: if g_gameFilesWritable -> MapDownloader.exe; else -> MapDownloaderUAC.exe
		info.lpFile = "MapDownloader.exe";
		if (now->isAsync)
			info.nShow = SW_HIDE;
		else info.nShow = SW_SHOW;
		info.cbSize = sizeof(SHELLEXECUTEINFOA);
		info.fMask = SEE_MASK_NOCLOSEPROCESS;
		info.hwnd = 0;
		//MessageBoxA(0,cwd,0,0);
		if (!ShellExecuteExA(&info)) {
			printf("\nFailed to start map downloader, error code %d\n", GetLastError());
			while (getchar() != '\n') {}
			return false;
		}
		now->hProcess = info.hProcess;

		WaitForSingleObject(info.hProcess, INFINITE);
		DWORD exitCode;
		ret = true;
		if (GetExitCodeProcess(info.hProcess, &exitCode)) {
			if (exitCode != 0) {
				printf("\nFailed to download map, error code: %d\n", (int)exitCode);
				ret = false;
			}
		}
	}
	if (!now->isAsync) {
		ILevelSystem *pLevelSystem = pGameFramework->GetILevelSystem();
		if (pLevelSystem) {
			pLevelSystem->Rescan();
		}
		ShowWindow(hwnd, SW_MAXIMIZE);
	}
	return ret;
}
void AsyncConnect(int id, AsyncData *obj) {
	//GetAsyncObj(ConnectStruct,now);
	ConnectStruct *now = (ConnectStruct *)obj;
	std::string content = "\\\\Error: Unknown error";
	if (now) {
		now->lock();
		std::string host = now->host;
		std::string page = now->page;
		Network::INetMethods method = now->method, http = now->http;
		unsigned short port = now->port;
		int timeout = now->timeout;
		bool alive = now->alive;
		now->unlock();
		content = Network::Connect(host, page, method, http, port, timeout, alive);
	}
	if (content.length() > 2) {
		if (content[0] == 0xFE && content[1] == 0xFF) content = content.substr(2);
	}
	obj->ret(content.c_str());
}
bool AsyncDownloadMap(int id, AsyncData *obj) {
	DownloadMapStruct *now = (DownloadMapStruct*)obj;
	if (now) {
		now->success = DownloadMapFromObject(now);
		now->ret(now->success);
		return now->success;
	}
	return false;
}
static void AsyncThread(){
	extern Mutex g_mutex;
	WSADATA data;
	WSAStartup(0x202, &data);
	while(true){
		WaitForSingleObject(gEvent,INFINITE);
		if(asyncQueue.size()){
			for (std::list<AsyncData*>::iterator it = asyncQueue.begin(); it != asyncQueue.end();it++) {
				g_mutex.Lock();
				AsyncData *obj = *it;
				if(obj && !obj->finished){
					obj->executing = true;
					g_mutex.Unlock();
					try {
						obj->exec();
					} catch (std::exception& ex) {
						printf("func/Unhandled exception: %s\n", ex.what());
					}
					g_mutex.Lock();
					obj->finished=true;
				}
				g_mutex.Unlock();
			}
		}
		ResetEvent(gEvent);
	}
	WSACleanup();
}
void GetClosestFreeItem(int *out){
	static AtomicCounter idx(0);
	*out = idx.increment();
}
#pragma endregion
