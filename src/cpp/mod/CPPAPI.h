#pragma once
#ifndef CPPAPI_H
#define CPPAPI_H
#include "Shared.h"
#ifdef USE_SDK
#include <IGameFramework.h>
#include <ISystem.h>
#include <IScriptSystem.h>
#include <IConsole.h>
#include <ILevelSystem.h>
#include <I3DEngine.h>
#include <IRenderer.h>
#include <IRendererD3D9.h>
#include <Windows.h>
#include "Mutex.h"
//#include <mutex>
#include "NetworkStuff.h"
#pragma region CPPAPIDefinitions
class CPPAPI : public CScriptableBase {
public:
	CPPAPI(ISystem*,IGameFramework*);
	~CPPAPI();
	int ConnectWebsite(IFunctionHandler* pH,char * host,char * page,int port,bool http11=false,int timeout=15,bool methodGet=true,bool alive=false);
	int FSetCVar(IFunctionHandler* pH,const char * cvar,const char *val);
	int AsyncConnectWebsite(IFunctionHandler* pH,char * host,char * page,int port,bool http11=false,int timeout=15,bool methodGet=true,bool alive=false);
	int GetIP(IFunctionHandler* pH,char* host);
	int GetMapName(IFunctionHandler* pH);
	int Random(IFunctionHandler* pH);
	int MapAvailable(IFunctionHandler *pH,const char *path);
	int DownloadMap(IFunctionHandler *pH,const char*,const char *);
	int GetLocalIP(IFunctionHandler* pH);
	int ApplyMaskAll(IFunctionHandler* pH,int mask,bool apply);
	int ApplyMaskOne(IFunctionHandler* pH,ScriptHandle,int,bool);
	int DoAsyncChecks(IFunctionHandler *pH);
	int MsgBox(IFunctionHandler* pH,const char *text,const char* title=0,int mask=MB_OK);
	//int ClearLayer(IFunctionHandler* pH,int layer);
protected:
	void RegisterMethods();
	ISystem				*m_pSystem;
	IScriptSystem		*m_pSS;
	IGameFramework		*m_pGameFW;
	HANDLE				thread;
};
extern IConsole *pConsole;
extern IGameFramework *pGameFramework;
extern ISystem *pSystem;
#pragma endregion

#pragma region AsyncStuff
#define MAX_ASYNC_QUEUE 255
struct AsyncData;

static void AsyncThread();
void AsyncConnect(int id, AsyncData *obj);
inline void GetClosestFreeItem(AsyncData **in, int *out);

struct AsyncData{
	int id;
	bool finished;
	bool executing;
	virtual void exec() {}
	virtual void onUpdate() {}
	virtual void postExec() {}
	virtual int callAsync(IFunctionHandler *pH=0) {
		extern Mutex g_mutex;
		g_mutex.Lock();
		extern HANDLE gEvent;
		extern AsyncData *asyncQueue[MAX_ASYNC_QUEUE];
		extern int asyncQueueIdx;
		GetClosestFreeItem(asyncQueue, &asyncQueueIdx);
		this->id = asyncQueueIdx;
		this->finished = false;
		this->executing = false;
		SetEvent(gEvent);
		asyncQueue[asyncQueueIdx] = this;
		g_mutex.Unlock();
		if (pH) {
			return pH->EndFunction(asyncQueueIdx);
		}
		return 0;
	}
	void ret(std::string what) {
		extern Mutex g_mutex;
		extern IScriptSystem *pScriptSystem;
		extern std::map<std::string, std::string> asyncRetVal;
		char outn[255];
		sprintf(outn, "AsyncRet%d", (int)id);
		pScriptSystem->SetGlobalAny(outn, what.c_str());
#ifdef DO_ASYNC_CHECKS
		g_mutex.Lock();
		asyncRetVal[std::string(outn)] = what;
		g_mutex.Unlock();
#endif
	}
	AsyncData():
		finished(false),
		executing(false){}
};

#ifdef IS6156DLL
#define AsyncReturn(what)\
	char outn[255];\
	sprintf(outn,"AsyncRet%d",(int)id);\
	gEnv->pSystem->GetIScriptSystem()->SetGlobalAny(outn,what)
#else
#define AsyncReturn(what)\
	extern IScriptSystem *pScriptSystem;\
	char outn[255];\
	sprintf(outn,"AsyncRet%d",(int)id);\
	pScriptSystem->SetGlobalAny(outn,what);\
	asyncRetVal[std::string(outn)] = what
#endif
#define GetAsyncObj(type,name) type *name=(type*)asyncQueue[id]
#define CreateAsyncCallLua(data)\
	GetClosestFreeItem(asyncQueue,&asyncQueueIdx);\
	data->id=asyncQueueIdx;\
	data->finished=false;\
	data->executing=false;\
	asyncQueue[asyncQueueIdx]=data;\
	SetEvent(gEvent);\
	return pH->EndFunction(asyncQueueIdx)
#define CreateAsyncCall(data)\
	GetClosestFreeItem(asyncQueue,&asyncQueueIdx);\
	data->id=asyncQueueIdx;\
	data->finished=false;\
	data->executing=false;\
	SetEvent(gEvent);\
	asyncQueue[asyncQueueIdx]=data

struct ConnectStruct : public AsyncData {
	char *host;
	char *page;
	Network::INetMethods method;
	Network::INetMethods http;
	unsigned short port;
	unsigned int timeout;
	bool alive;
	virtual void exec() {
		AsyncConnect(this->id, (AsyncData*)this);
	}
};
struct DownloadMapStruct : public AsyncData{
	const char *mapn;
	const char *mapdl;
	bool success;
	virtual void exec() {
		AsyncConnect(this->id, (AsyncData*)this);
	}
};
#pragma endregion
#endif
#endif