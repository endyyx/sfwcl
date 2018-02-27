#include "IntegrityService.h"

bool IntegrityService::Init(IGameObject *pGameObject) {
	SetGameObject(pGameObject);
	if (!GetGameObject()->BindToNetwork())
		return false;
	return true;
}
IMPLEMENT_RMI(IntegrityService, SvRequestMemoryCheck) {
	/* none of our business here */
	return true;
}
IMPLEMENT_RMI(IntegrityService, ClRequestMemoryCheck) {
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