#pragma once

#define WIN32_LEAN_AND_MEAN
#include "Shared.h"
#include <Windows.h>

//Just a really simple Mutex class, we don't need whole <mutex> of C++11 adding 200kB overhead to binary
struct Mutex {
	CRITICAL_SECTION cs;
	const char *szName;
	Mutex(const char *name=0) {
		szName = name;
#ifdef THREAD_SAFE
		ZeroMemory(&cs, sizeof(cs));
		InitializeCriticalSection(&cs);
#endif
	}
	~Mutex() {
#ifdef THREAD_SAFE
		DeleteCriticalSection(&cs);
#endif
	}
	bool Lock() {
#ifdef THREAD_SAFE
		EnterCriticalSection(&cs);
#endif
		return true;
	}
	void Unlock() {
#ifdef THREAD_SAFE
		LeaveCriticalSection(&cs);
#endif
	}
};
struct SimpleLock {
	Mutex *mptr;
	SimpleLock(Mutex mtx) : mptr(&mtx) {
		mtx.Lock();
	}
	~SimpleLock() {
		if(mptr)
			mptr->Unlock();
	}
};
