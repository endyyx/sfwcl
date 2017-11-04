#include "NetworkStuff.h"
#include <wininet.h>
#include <fstream>
#include <sstream>

static size_t curlCB(void *c, size_t sz, size_t n, void *out){
    ((std::string*)out)->append((char*)c, sz*n);
    return sz*n;
}

namespace Network{
	struct AliveSock{
		int sock;
		sockaddr_in si;
	};
	std::string ReturnError(std::string error,int sock){ 
        closesocket(sock); 
        return error; 
    } 
	static std::map<std::string,AliveSock> aliveSocks;
	std::string Connect(std::string host, std::string page, INetMethods method, INetMethods http, int port, int timeout, bool alive) {

		char *buffer = new char[16384];
		try {

			std::string script = page;
			std::string params = "";

			size_t pos = script.find('?');

			try {

				if (pos != std::string::npos) {
					script = script.substr(0, pos);
					params = page.substr(pos + 1);
				}
			}
			catch (std::exception& ex) {
				UNREFERENCED_PARAMETER(ex);
				script = page;
				params = "";
			}

			//MessageBoxA(0, (std::string("Page: ")+page+"\r\n"+std::string("Script: ") + script + "\r\nParams: " + params).c_str(), 0, 0);

			const char *headers = "Content-Type: application/x-www-form-urlencoded";
			const char *form = params.c_str();
			LPCSTR accept[] = { "*/*" };

			HINTERNET hSession = InternetOpen("SSMSafeWriting/2.8.1+",
				INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
			HINTERNET hConnect = InternetConnect(hSession,host.c_str(),
				INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);

			if (timeout > 1000) timeout /= 1000;
			DWORD timeo = timeout * 1000;

			InternetSetOption(hConnect, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeo, sizeof timeo);
			InternetSetOption(hConnect, INTERNET_OPTION_SEND_TIMEOUT, &timeo, sizeof timeo);
			InternetSetOption(hConnect, INTERNET_OPTION_CONNECT_TIMEOUT, &timeo, sizeof timeo);


			HINTERNET hRequest = HttpOpenRequest(hConnect, method == INetPost ? "POST" : "GET",
				(script+(method==INetGet?std::string("?"+params):std::string(""))).c_str(), NULL, NULL, accept, 0, 1);
			BOOL res = HttpSendRequest(hRequest, headers, (DWORD)strlen(headers), (void*)form, (DWORD)strlen(form));
			std::stringstream ss;
			if (res) {

				DWORD dwBytes = 0;
				while (InternetReadFile(hRequest, buffer, 16384, &dwBytes))
				{
					if (dwBytes <= 0) break;
					ss << std::string(buffer, buffer + dwBytes);
				}
				//MessageBoxA(0, ss.str().c_str(), "SUCCESS", 0);
			}
			else {
				ss << "\\\\Error: WinInet code: " << GetLastError();
			}
			//else MessageBoxA(0, "FAIL", 0, 0);
			InternetCloseHandle(hRequest);
			InternetCloseHandle(hConnect);
			InternetCloseHandle(hSession);

			delete[] buffer;
			buffer = 0;

			return ss.str();
		}
		catch (std::exception& ex) {
			//MessageBoxA(0, ex.what(), "FATAL ERROR", 0);
			if (buffer) {
				delete[] buffer;
				buffer = 0;
			}
			return std::string("\\\\Error: Fatal error: ") + ex.what();
		}
		return "\\\\Error: unknown error";
	}
	std::string ReturnError(std::string error){
		return error;
	}
	void GetIP(const char * ihostname,char *out){
		char * hostname=strtok(const_cast<char *>(ihostname),":");
		int result=0;
		in_addr iaddr;
		hostent *host;
		host=gethostbyname(hostname);
		if(!host)
			strcpy(out,hostname);
		else {
			iaddr.s_addr=*(unsigned long*)host->h_addr;
			char *t_ip;
			t_ip=inet_ntoa(iaddr);
			strcpy(out,t_ip);
		}
	}
	char* GetIP(const char * ihostname){
		char * hostname=strtok(const_cast<char *>(ihostname),":");
		int result=0;
		in_addr iaddr;
		hostent *host;
		host=gethostbyname(hostname);
		if(!host)
			return (char*)ihostname;
		else {
			iaddr.s_addr=*(unsigned long*)host->h_addr;
			char *t_ip;
			t_ip=inet_ntoa(iaddr);
			return t_ip;
		}
	}
}
