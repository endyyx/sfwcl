#include "IntegrityService.h"
#include <IScriptSystem.h>
//#include <IActorSystem.h>
#include <Windows.h>

CIntegrityService* CIntegrityService::instance = 0;

bool CIntegrityService::Init(IGameObject *pGameObject) {
	if (!pGameObject) return false;
	SetGameObject(pGameObject);
	MessageBoxA(0, "IntegrityService::Init", 0, 0);
	if (!GetGameObject()->BindToNetwork())
		return false;
	return true;
}
IMPLEMENT_RMI(CIntegrityService, SvRequestMemoryCheck) {
	/* none of our business here */
	return true;
}
IMPLEMENT_RMI(CIntegrityService, ClRequestMemoryCheck) {
#ifdef _WIN64
	unsigned long long addr = 0;
	addr |= params.addr1;
	addr <<= 32;
#else
	unsigned long addr = 0;
#endif
	addr |= params.addr2;
	void *ptr = (void*)addr;
	std::string signature = signMemory(ptr, params.len, params.payload);
	MemoryCheckParams back;
	back.id = params.id;
	back.len = params.len;
	back.addr1 = params.addr1;
	back.addr2 = params.addr2;
	back.payload = signature.c_str();
	GetGameObject()->InvokeRMI(SvRequestMemoryCheck(), back, eRMI_ToServer);
	return true;
}
IMPLEMENT_RMI(CIntegrityService, SvOnReceiveMessage) {
	/* none of our business */
	MessageBoxA(0, "Received message", 0, 0);
	MessageBoxA(0, params.payload.c_str(), 0, 0);
	return true;
}
IMPLEMENT_RMI(CIntegrityService, ClOnReceiveMessage) {
	//client side
	extern IScriptSystem *pScriptSystem;
	pScriptSystem->BeginCall("OnMessage");
	pScriptSystem->PushFuncParam(params.id.c_str());
	pScriptSystem->PushFuncParam(params.payload.c_str());
	pScriptSystem->EndCall();
	return true;
}