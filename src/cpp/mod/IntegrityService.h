#pragma once
#ifndef __INTEGRITY_SERVICE_H__
#define __INTEGRITY_SERVICE_H__
#include "Shared.h"
#include <IGameFramework.h>
#define GameWarning(...) /* do nothing */
#include <IGameObject.h>
#include <ISerialize.h>

#define VERIFY_SERVER	"m.crymp.net"
#define VERIFY_PORT		2900

struct IntegrityService :
	public CGameObjectExtensionHelper<IntegrityService, IGameObjectExtension, 64> {
	struct MemoryCheckParams {
		MemoryCheckParams() {};
		MemoryCheckParams(int a1, int a2, int l) : addr1(a1), addr2(a2), len(l) {};
		int addr1;
		int addr2;
		int len;
		string payload;
		string id;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("addr1", addr1, 'adr1');
			ser.Value("addr2", addr2, 'adr2');
			ser.Value("length", len, 'leng');
			ser.Value("payload", payload, 'pald');
			ser.Value("id", id, 'id__');
		}
	};
	IntegrityService() {
		
	}
	virtual bool Init(IGameObject *pGameObject);
	virtual void PostInit(IGameObject * pGameObject) {}
	virtual void InitClient(int channelId) {}
	virtual void PostInitClient(int channelId) {}
	virtual void Release() {}
	virtual void FullSerialize(TSerialize ser) {}
	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) { return true; }
	virtual void PostSerialize() {}
	virtual void SerializeSpawnInfo(TSerialize ser) {}
	virtual ISerializableInfoPtr GetSpawnInfo() { return 0; }
	virtual void Update(SEntityUpdateContext& ctx, int updateSlot) {}
	virtual void HandleEvent(const SGameObjectEvent&) {}
	virtual void ProcessEvent(SEntityEvent&) {}
	virtual void SetChannelId(uint16 id) {}
	virtual void SetAuthority(bool auth) {}
	virtual void PostUpdate(float frameTime) {}
	virtual void PostRemoteSpawn() {}
	virtual void GetMemoryStatistics(ICrySizer * s) {}

	DECLARE_SERVER_RMI_NOATTACH(SvRequestMemoryCheck, MemoryCheckParams, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_NOATTACH(ClRequestMemoryCheck, MemoryCheckParams, eNRT_ReliableOrdered);
};

#endif