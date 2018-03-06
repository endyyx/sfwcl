#pragma once
#include <vector>
struct RPC {
	static RPC *instance;
	unsigned long ip;
	int port;
	int sock;
	int expect;
	int received;
	unsigned char *buffer;
	bool active;
	bool head;
	static RPC* getInstance() {
		if (!instance) instance = new RPC();
		return instance;
	}
	RPC() : sock(0) {}
	~RPC() {
		if (buffer) {
			delete[] buffer;
			buffer = 0;
		}
	}
	void establish(unsigned long ip, int port);
	void shutdown();
	void execute();
	void send(const char *data, int len);
	void sendMessage(const char *id, std::vector<const char*>& args);
	static void recv_thread(RPC *self);
};
