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
typedef void (*AsyncFuncType)(int);
struct AsyncData{
	int id;
	bool finished;
	void *func;
	AsyncData():finished(false){}
};
#define MAX_ASYNC_QUEUE 255
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
	pScriptSystem->SetGlobalAny(outn,what);
#endif
#define GetAsyncObj(type,name) type *name=(type*)asyncQueue[id]
#define CreateAsyncCallLua(funct,data)\
	GetClosestFreeItem(asyncQueue,&asyncQueueIdx);\
	data->id=asyncQueueIdx;\
	data->finished=false;\
	data->func=(void*)funct;\
	asyncQueue[asyncQueueIdx]=data;\
	SetEvent(gEvent);\
	return pH->EndFunction(asyncQueueIdx)
#define CreateAsyncCall(funct,data)\
	GetClosestFreeItem(asyncQueue,&asyncQueueIdx);\
	data->id=asyncQueueIdx;\
	data->finished=false;\
	data->func=(void*)funct;\
	SetEvent(gEvent);\
	asyncQueue[asyncQueueIdx]=data;

static AsyncData *asyncQueue[MAX_ASYNC_QUEUE];
static int asyncQueueIdx=0;
static void AsyncThread();
inline void GetClosestFreeItem(AsyncData **in,int *out);
struct ConnectStruct : public AsyncData{
	char *host;
	char *page;
	Network::INetMethods method;
	Network::INetMethods http;
	unsigned short port;
	unsigned int timeout;
	bool alive;
};
struct DownloadMapStruct : public AsyncData{
	const char *mapn;
	const char *mapdl;
	bool success;
};
void AsyncConnect(int id);
#pragma endregion
#endif
#endif