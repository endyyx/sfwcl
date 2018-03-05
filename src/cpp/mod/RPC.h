#pragma once

struct RPC {
	static RPC *instance;
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
	void send(char *data, int len);
	static void recv_thread(RPC *self);
};
