#pragma once
#ifndef INET_CLASS
#define INET_CLASS
#include "Shared.h"
#ifdef USE_SDK
#include <string>
#include <map>
#include <Windows.h>
#pragma comment(lib,"Ws2_32")
namespace Network{
	enum INetMethods{
		INetGet=0xf25,INetPost,INetHTTP10,INetHTTP11
	};
	std::string Connect(std::string host,std::string page,INetMethods method=INetGet,INetMethods http=INetHTTP10,int port=80,int timeout=15,bool alive=false);
	std::string ReturnError(std::string error);
	void GetIP(const char*,char *);
	char *GetIP(const char*);
}
#endif
#endif