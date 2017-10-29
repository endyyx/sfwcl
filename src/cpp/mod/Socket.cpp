#include "Socket.h"
#ifdef USE_SDK

Socket::Socket(ISystem *pSystem, IGameFramework *pGameFramework)
	:	m_pSystem(pSystem),
		m_pSS(pSystem->GetIScriptSystem()),
		m_pGameFW(pGameFramework)
{
	Init(m_pSS, m_pSystem);
	SetGlobalName("Socket");
	RegisterMethods();
}
Socket::~Socket(){
}
void Socket::RegisterMethods(){
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &Socket::
	SetGlobalName("Socket");
	SCRIPT_REG_TEMPLFUNC(socket,"tcp");
	SCRIPT_REG_TEMPLFUNC(connect,"sock, host, port");
	SCRIPT_REG_TEMPLFUNC(send,"sock, buffer, len");
	SCRIPT_REG_TEMPLFUNC(recv,"sock, len");
	SCRIPT_REG_TEMPLFUNC(close,"sock");
	SCRIPT_REG_TEMPLFUNC(error,"");
}

int Socket::socket(IFunctionHandler *pH,bool tcp){
	int sock = ::socket(AF_INET, tcp?SOCK_STREAM:SOCK_DGRAM,tcp?IPPROTO_TCP:IPPROTO_UDP);
	return pH->EndFunction(sock,WSAGetLastError());
}
int Socket::connect(IFunctionHandler *pH,int sock,const char *hostname,int port){
	sockaddr_in sa;
	ZeroMemory(&sa,sizeof(sa));
	hostent *host;
	host=gethostbyname(hostname);
	if(!host) return pH->EndFunction(false,WSAGetLastError(),1);
	sa.sin_addr.s_addr=*(unsigned long*)host->h_addr;
	sa.sin_port=htons(port);
	sa.sin_family=AF_INET;
	if(::connect(sock, (sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR){
		return pH->EndFunction(false,WSAGetLastError(),2);
	}
	return pH->EndFunction(true);
}
int Socket::send(IFunctionHandler *pH,int sock,const char *buffer,int len){
	return pH->EndFunction(::send(sock, buffer, len, 0),WSAGetLastError());
}
int Socket::recv(IFunctionHandler *pH,int sock, int len){
	static char recvBuffer[1024*1024];
	memset(recvBuffer, 0, sizeof(recvBuffer));
	int retlen = ::recv(sock, recvBuffer, len, 0);
	return pH->EndFunction(retlen, recvBuffer, WSAGetLastError());
}
int Socket::close(IFunctionHandler *pH,int sock){
	::close(sock);
	return pH->EndFunction(WSAGetLastError());
}
int Socket::error(IFunctionHandler *pH){
	LPVOID lpMsgBuf;
	DWORD dw = GetLastError(); 
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );
	std::string str = std::to_string((long long)WSAGetLastError())+"/"+((const char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
	return pH->EndFunction(str.c_str(),(int)dw);
}

#endif