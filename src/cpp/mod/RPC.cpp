#include "CPPAPI.h"
#include "RPC.h"
#include <string.h>

#include <winsock2.h>
#include <windows.h>
#include <winsock.h>

#define BUFFER_SIZE 32768

void RPC::establish(unsigned long ip, int port) {
	if (sock) {
		closesocket(sock);
		sock = 0;
		active = false;
	}
	this->ip = ip;
	this->port = port&0xFFFF;
	buffer = new unsigned char[BUFFER_SIZE];
	CreateThread(0, 16384, (LPTHREAD_START_ROUTINE)&RPC::recv_thread, (LPVOID)this, 0, 0);
}
void RPC::shutdown() {
	closesocket(sock);
	sock = 0;
	active = false;
}
void RPC::execute() {
	std::vector<std::string> args;
	std::string arg = "";
	for (int i = 0; i < received; i++) {
		char n = (char)((buffer[i] ^ 0x5A)&0xFF);
		if (n == 0) {
			args.push_back(arg);
			arg = "";
		}
		else arg += n;
	}
	RPCEvent *evt = new RPCEvent(args);
	evt->callAsync();
}
void RPC::send(const char *data, int len) {
	if (active) {
		::send(sock, (char*)&len, 4, 0);
		::send(sock, data, len, 0);
	}
}
void RPC::sendMessage(const char *method, std::vector<const char*>& args) {
	std::string str = method;
	str += '\0';
	for (auto& arg : args) {
		str += arg; str += '\0';
	}
	for (size_t i = 0; i < str.size(); i++)
		str[i] = (str[i] ^ 0x5A)&0xFF;
	send(str.c_str(), str.size());
}
void RPC::recv_thread(RPC *self) {
	WSADATA wsa;
	WSAStartup(0x202, &wsa);
	self->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in sa;
	ZeroMemory(&sa, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(self->ip);
	sa.sin_port = htons(self->port);
	if (!connect(self->sock, (const sockaddr*)&sa, sizeof(sockaddr))) {
		self->active = true;
		self->head = true;
		self->expect = 4;
		self->received = 0;
	} else {
		self->active = false;
		return;
	}
	while (true) {
		int n = recv(self->sock, (char*)self->buffer + self->received, self->expect, 0);
		if (n <= 0) {
			self->shutdown();
			return;
		}
		else {
			self->expect -= n; self->received += n;
			if (self->expect == 0) {
				if (self->head) {
					int len = 0;
					len |= self->buffer[3]; len <<= 8;
					len |= self->buffer[2]; len <<= 8;
					len |= self->buffer[1]; len <<= 8;
					len |= self->buffer[0];
					self->expect = len;
					self->received = 0;
					if (len >= BUFFER_SIZE - 4) {
						self->shutdown();
						return;
					}
					memset(self->buffer, 0, BUFFER_SIZE);
					self->head = false;
				}
				else {
					self->execute();
					self->expect = 4;
					self->received = 0;
					self->head = true;
				}
			}
		}
	}
}