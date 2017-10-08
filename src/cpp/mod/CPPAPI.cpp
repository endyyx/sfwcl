#include "CPPAPI.h"
#ifdef USE_SDK
#include <IEntity.h>
#include <IEntitySystem.h>
#include <IVehicleSystem.h>
#include <IGameObjectSystem.h>
#include <CryThread.h>

#pragma region CPPAPI

static HANDLE gEvent;

extern void ToggleLoading(const char *text,bool loading);

CPPAPI::CPPAPI(ISystem *pSystem, IGameFramework *pGameFramework)
	:	m_pSystem(pSystem),
		m_pSS(pSystem->GetIScriptSystem()),
		m_pGameFW(pGameFramework)
{
	Init(m_pSS, m_pSystem);
	for(int i=0;i<MAX_ASYNC_QUEUE;i++)
		asyncQueue[i]=0;
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
}
int CPPAPI::FSetCVar(IFunctionHandler* pH,const char * cvar,const char *val){
#ifdef IS6156DLL
	if(gEnv->pConsole->GetCVar(cvar)!=NULL)
		gEnv->pConsole->GetCVar(cvar)->ForceSet(val);
	return pH->EndFunction(true);
#else
	if(ICVar *cVar=pConsole->GetCVar(cvar))
		cVar->ForceSet(val);
	return pH->EndFunction(true);
#endif
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
#ifdef IS6156DLL
	return pH->EndFunction(gEnv->pGame->GetIGameFramework()->GetLevelName());
#else
	return pH->EndFunction(pGameFramework->GetLevelName());
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
		for(int l = 0; l < pLevelSystem->GetLevelCount(); ++l){
			ILevelInfo *pLevelInfo = pLevelSystem->GetLevelInfo(l);
			if(pLevelInfo){
				if(stricmp(pLevelInfo->GetName(),path)==0){
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
	sprintf_s(params, "\"%s\" \"%s\" \"%s\"", mapn, mapdl, cwd);
	SHELLEXECUTEINFOA info;
	ZeroMemory(&info, sizeof(SHELLEXECUTEINFOA));
	info.lpDirectory = cwd;
	info.lpParameters = params;
	info.lpFile = "MapDownloader.exe";
	info.nShow = SW_SHOW;
	info.cbSize = sizeof(SHELLEXECUTEINFOA);
	info.fMask = SEE_MASK_NOCLOSEPROCESS;
	info.hwnd = 0;
	//MessageBoxA(0,cwd,0,0);
	if (!ShellExecuteExA(&info)) {
		printf("\nFailed to start map downloader, error code %d\n", GetLastError());
		while (getchar() != '\n') {}
		return false;
	}
	WaitForSingleObject(info.hProcess, INFINITE);
	DWORD exitCode;
	bool ret = true;
	if (GetExitCodeProcess(info.hProcess, &exitCode)) {
		if (exitCode != 0) {
			printf("\nFailed to download map, error code: %d\n", (int)exitCode);
			ret = false;
		}
	}
	ILevelSystem *pLevelSystem = pGameFramework->GetILevelSystem();
	if (pLevelSystem) {
		pLevelSystem->Rescan();
	}
	ShowWindow(hwnd, SW_MAXIMIZE);
	return ret;
}
bool AsyncDownloadMap(int id){
	GetAsyncObj(DownloadMapStruct,now);
	if(now){
		now->success=DownloadMapFromObject(now);
		//ToggleLoading("Map downloaded",false);
		return now->success;
	}
	return false;
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
void AsyncConnect(int id){
	GetAsyncObj(ConnectStruct,now);
	std::string content="Error: Unknown error";
	if(now)
		content=Network::Connect(now->host,now->page,now->method,now->http,now->port,now->timeout,now->alive);
	AsyncReturn(content.c_str());
}
int CPPAPI::AsyncConnectWebsite(IFunctionHandler* pH,char * host,char * page,int port,bool http11,int timeout,bool methodGet,bool alive){
	using namespace Network; 
	ConnectStruct *now=new ConnectStruct;
	if(now){
		now->host=host;
		now->page=page;
		now->method=methodGet?INetGet:INetPost;
		now->http=http11?INetHTTP11:INetHTTP10;
		now->port=port;
		now->timeout=timeout;
		now->alive=alive;
		CreateAsyncCallLua(AsyncConnect,now);
	}
	return pH->EndFunction();
}
#pragma endregion

#pragma region AsyncStuff
static void AsyncThread(){
	while(true){
		WaitForSingleObject(gEvent,INFINITE);
		if(asyncQueue){
			for(int i=0;i<MAX_ASYNC_QUEUE;i++){
				AsyncData *obj=asyncQueue[i];
				if(obj && !obj->finished){
					if(obj->func){
						AsyncFuncType func=(AsyncFuncType)obj->func;
						func(obj->id);
						obj->finished=true;
						try{
							delete obj;
						} catch(std::exception& ex){
							printf("Unhandled exception: %s",ex.what());
						}
						asyncQueue[i]=0;
					}
				}
			}
			GetClosestFreeItem(asyncQueue,&asyncQueueIdx);
		}
		ResetEvent(gEvent);
	}
}
void GetClosestFreeItem(AsyncData **in,int *out){
	*out=0;
	if(in){
		if(!in[0]){
			return;
		}
		for(int i=1;i<MAX_ASYNC_QUEUE;i++){
			if((in[i])==0){
				*out=i;
				break;
			}
		}
	}
}
#pragma endregion
#endif