#pragma once
#ifndef __MUTEX_H__
#define __MUTEX_H__
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//Just a really simple Mutex class, we don't need whole <mutex> of C++11 adding 200kB overhead to binary
struct Mutex {
	HANDLE hMutex;
	DWORD threadId;
	const char *szName;
	Mutex(const char *name=0) {
		szName = name;
		hMutex = CreateMutexA(0, false, name);
	}
	~Mutex() {
		if (hMutex) {
			CloseHandle(hMutex);
		}
	}
	bool Lock(DWORD timeo = INFINITE) {
		DWORD res = WaitForSingleObject(hMutex, timeo);
		threadId = GetCurrentThreadId();
		if (res == WAIT_TIMEOUT) return false;
		return res != WAIT_FAILED;
	}
	void Unlock() {
		ReleaseMutex(hMutex);
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
#endif