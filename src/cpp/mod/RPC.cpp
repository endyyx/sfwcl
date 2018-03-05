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
	sockaddr_in sa;
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ZeroMemory(&sa, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(ip);
	sa.sin_port = htons(port);
	buffer = new unsigned char[BUFFER_SIZE];
	if (!connect(sock, (const sockaddr*)&sa, sizeof(sockaddr))) {
		active = true;
		head = true;
		expect = 4;
		received = 0;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&RPC::recv_thread, (LPVOID)this, 0, 0);
	}
	else active = false;
}
void RPC::shutdown() {
	closesocket(sock);
	sock = 0;
	active = false;
}
void RPC::execute() {
	const char *a = (const char*)buffer, *b = 0, *c = 0, *d = 0, *e = 0;
	b = strchr(a, 0);
	if (b) c = strchr(++b, 0);
	if (c) d = strchr(++c, 0);
	if (d) e = strchr(++d, 0);
	if (e) e++;
	RPCEvent *evt = new RPCEvent(a, b, c, d, e);
	evt->callAsync();
}
void RPC::send(char *data, int len) {
	if (active) {
		::send(sock, (char*)&len, 4, 0);
		::send(sock, data, len, 0);
	}
}
void RPC::recv_thread(RPC *self) {
	WSADATA wsa;
	WSAStartup(0x202, &wsa);
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